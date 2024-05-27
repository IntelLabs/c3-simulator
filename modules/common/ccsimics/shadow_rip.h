/*
 Copyright Intel Corporation
 SPDX-License-Identifier: MIT
*/

#ifndef MODULES_COMMON_CCSIMICS_SHADOW_RIP_H_
#define MODULES_COMMON_CCSIMICS_SHADOW_RIP_H_

#include <memory>
#include <string>
#include <utility>
extern "C" {
#include <xed-interface.h>  // NOTE! The xed header is C-only
}
#include "ccsimics/simics_connection.h"
#include "ccsimics/simics_util.h"
#include "ccsimics/xed_util.h"
#include "crypto/cc_encoding.h"
#include "malloc/cc_globals.h"

#ifndef SLOT_SIZE_IN_BITS
#define SLOT_SIZE_IN_BITS 3
#define ICV_ADDR_MASK 0xFFFFFFFFFFFFFFF8
#endif  // SLOT_SIZE_IN_BITS

namespace ccsimics {

/**
 * @brief Shadow-rip implementation
 *
 * Implements support for shadow-rip on LEA and memory accesses. RIP-relative
 * addresses are modifyied such that the fixed-portion of the address is
 * replaced from the gsrip if the resulting address would be within the power
 * slot of the gsrip.
 *
 * Instruction decode callbacks that:
 *  1) Modify LEA behavior directly to return gsrip-replace address
 *  2) Memory access callback that detects rip-relative memory accesses, but
 *     otherwise reverts back to default instruction semantics.
 *  3) RET / CALL callbacks to update gsrip automatically
 *
 * The gsrip value is stored within the CPU state manageable via the struct
 * cc_context and Context. The state also includes an enable bit for control.
 *
 * Addresses for memory accesses are not directly modified, instead, a separate
 * address_before callback is epxected to trigger shadow-rip replacement if the
 * instruciton decode callback has marked the instruction as rip-relative in
 * this cycle. Note that the is_rip_rel_ bit must be reset externally as the
 * instruction decode callback is ran only for matching opcodes!
 *
 * @tparam CtxTy
 * @tparam PtrEncTy
 */

#define gen_xedd_pair(_self, _xedd)                                            \
    reinterpret_cast<void *>(                                                  \
            new xedd_pair(_self, new xed_decoded_inst_t(_xedd)))
#define xedd_pair_free_lambda                                                  \
    [](auto, auto, lang_void *cb_data) -> void {                               \
        auto *d = reinterpret_cast<xedd_pair *>(cb_data);                      \
        delete d->second; /* Delete xedd copy */                               \
        delete d;         /* Delete the cb_data pair */                        \
    }
#define xedd_pair_self(pair) (reinterpret_cast<xedd_pair *>(pair)->first)
#define xedd_pair_xedd(pair) (reinterpret_cast<xedd_pair *>(pair)->second)
#define xedd_pair_call_cb(pair, cb_func)                                       \
    (xedd_pair_self(pair)->cb_func(xedd_pair_xedd(pair)))
#define xedd_pair_emul_callback(cb_func)                                       \
    ([](auto, auto, auto *d) -> cpu_emulation_t {                              \
        return xedd_pair_call_cb(d, cb_func);                                  \
    })

template <typename CcTy, typename ConTy, typename CtxTy> class ShadowRip final {
    using SelfTy = ShadowRip<CcTy, ConTy, CtxTy>;
    using xedd_pair = std::pair<SelfTy *, xed_decoded_inst_t *>;

    // Unconditionally enable debug messages for this class
    static constexpr bool kDebugAlways = false;

    // Extensive output on callback entry and such
    static constexpr bool kTrace = false;

    // Enables extra debug output for exception callback
    static constexpr bool kTraceExceptionAfter = false;

    // Enables extra debug output for SMM entry / leave callback
    static constexpr bool kTraceSMM = false;

    // Break simulation whenever the gsrip value is changed
    static constexpr bool kBreakOnGsripChange = false;

    // Break simulation to-be-gsripped LA is outside GSRIP power slot
    static constexpr bool kBreakOnGsripEscape = false;

    // Raise fault if to-be-gsripped LA is outside GSRIP power slot
    static constexpr bool kFaultOnGsripEscape = true;

    // Update GSRIP on jmp instructions (in addition to calls)
    static constexpr bool kUpdateGsripOnJMP = true;

 public:
    static const xed_machine_mode_enum_t kXedMMode = XED_MACHINE_MODE_LONG_64;
    static const xed_address_width_enum_t kXedAddrWidth = XED_ADDRESS_WIDTH_64b;

 private:
    CcTy *cc_;
    ConTy *con_;
    CtxTy *ctx_;

    bool is_rip_rel_ = false;
    bool need_ra_fixup_ = false;
    bool need_cp_fixup_ = false;
    int reg_fixup_ = -1;
    uint64_t reg_fixup_val_ = 0;
    uint64_t old_gsrip_ = 0;
    uint64_t original_cp_value_ = 0;
    uint64_t original_cp_addr_ = 0;
    uint64_t rip_rel_val_ = 0;
    bool xed_agen_rip_replace_ = false;

    // Keep track of integrity enabling when we disable it for a cycle
    bool mem_call_integrity_hack_ = false;

 public:
    explicit ShadowRip(CcTy *cc, ConTy *con, CtxTy *ctx)
        : cc_(cc), con_(con), ctx_(ctx) {}

    inline auto is_enabled() { return ctx_->get_gsrip_enabled(); }

    /**
     * @brief Register Simics callbacks
     */
    inline void register_callbacks();

    /**
     * @brief Simics LEA callback
     *
     * This assumes the instruction_decode callback has also been registered
     * as it relies on it to detect rip-relative memory accesses.
     */
    inline uint64_t address_before_shim(logical_address_t la,
                                        address_handle_t *);

    void instruction_before_cb(instruction_handle_t *handle) {
        reset_internal_state();
    }

    void instruction_after_cb(instruction_handle_t *handle);

    void exception_before_cb(exception_handle_t *eh) { reset_internal_state(); }

    void exception_after_cb(exception_handle_t *eh);

    void smm_enter_before_cb();

    void smm_leave_after_cb();

    /**
     * @brief The instruction decode callback
     *
     * NOTE: After an instruction is detected, it a separation emulation
     * callback will be registered such that it is is directly called for
     * subsequent encounters of the same instruction. That is, for any specific
     * opcode, this may be called only once, after which the emulation callback
     * is directly called instead.
     */
    inline int instruction_decode(decoder_handle_t *decoder_handle,
                                  instruction_handle_t *instr_handle);

    /**
     * @brief Emulation callback for RIP-relative LEAs
     */
    inline cpu_emulation_t emul_lea(xed_decoded_inst_t *xedd);

    /**
     * @brief Simics RIP-relative memory accesses callback
     *
     * Note that this doesn't do anything beyond setting a state bit so that
     * we can then modify only the final linear address in the address_before
     * callback that is ran before the actual store / load happens.
     */
    inline cpu_emulation_t emul_mem_access(xed_decoded_inst_t *xedd);

    /**
     * @brief Simics CALL callback
     */
    inline cpu_emulation_t emul_call(xed_decoded_inst_t *xedd) {
        return emul_call_or_jmp(xedd, true);
    }

    /**
     * @brief Simics JMP callback
     */
    inline cpu_emulation_t emul_jmp(xed_decoded_inst_t *xedd) {
        return emul_call_or_jmp(xedd, false);
    }

    /**
     * @brief Simics CALL and JMP callback implementation
     */
    inline cpu_emulation_t emul_call_or_jmp(xed_decoded_inst_t *xedd,
                                            const bool is_call = true);

    /**
     * @brief Simics RET callback
     */
    inline cpu_emulation_t emul_ret(xed_decoded_inst_t *xedd);

    /**
     * @brief XED register-read callback with shadow-rip replacement
     */
    inline uint64_t emul_lea_reg_xed_cb(xed_reg_enum_t xed_reg,
                                        xed_bool_t *err);

    /**
     * @brief Get gsrip as decoded LA
     */
    inline uint64_t get_decoded_shadow_rip(uint64_t gsrip) {
        return cc_->decode_pointer(gsrip);
    }

 private:
    /**
     * @brief Update target of branch instruction with in-mem operand
     *
     * @return true if handled as in-mem operand
     * @return false if not handled
     */
    inline bool update_mem_branch_target(xed_decoded_inst_t *xedd,
                                         const bool is_call);

    /**
     * @brief Update target of branch instruction with register operand
     *
     * @return true if handled as register operand
     * @return false if not handled
     */
    inline bool update_reg_branch_target(xed_decoded_inst_t *xedd,
                                         const bool is_call);

    inline uint64_t decode_ptr(const uint64_t call_target) {
        return decode_ptr_and_update_gsrip(call_target, false);
    }

    /**
     * @brief Decode pointer, and also use CA to configure gsrip
     *
     * @param call_target Call target as LA CA
     * @param do_gsrip_update If false, disable gsrip update
     * @return uint64_t Decoded LA
     */
    inline uint64_t decode_ptr_and_update_gsrip(const uint64_t call_target,
                                                bool do_gsrip_update = true);

    /**
     * @brief Substitute LA fixed bits with given GSRIP
     *
     * NOTE: Does not modify LA if outside GSRIP power slot!
     */
    inline uint64_t replace_shadow_rip(const uint64_t la, const uint64_t gsrip);

    /**
     * @brief Substitute LA fixed bits with current GSRIP
     *
     * NOTE: Does not modify LA if outside GSRIP power slot!
     */
    inline uint64_t replace_shadow_rip(const uint64_t la);

    /**
     * @brief XED agent callback for register value reads during decode
     *
     * NOTE: This will raise a fault on target on failure
     */
    inline bool xed_agen_or_fault(xed_decoded_inst_t *xedd,
                                  uint64_t memop_index, void *context,
                                  uint64_t *dst);

    /**
     * @brief Convert XED reg to Simics reg, or raise fault on target
     *
     * @param xed_reg
     * @return int Simics reg, or X86_Reg_Id_Not_Used on failure
     */
    inline int convert_xed_reg_or_fault(xed_reg_enum_t xed_reg);

    /**
     * @brief Get configure GSRIP from C3 context
     */
    inline uint64_t get_shadow_rip() { return ctx_->get_gsrip(); }

    /**
     * @brief Set GSRIP, and update C3 context
     */
    inline void set_shadow_rip(uint64_t new_gsrip);

    inline bool dbg() { return kDebugAlways || this->con_->debug_on; }

    /**
     * @brief Raise a fault with stack trace, and fault if break_on_decode_fault
     */
    inline void fault_and_break_if_enabled(const char *msg) const {
        con_->raise_fault_with_stacktrace(msg, con_->break_on_decode_fault);
    }

    /**
     * @brief Check if xedd needs RET emulation
     */
    inline bool need_emul_ret(const xed_decoded_inst_t *xedd,
                              const xed_category_enum_t instr_cls) const {
        return (instr_cls == XED_CATEGORY_RET);
    }

    /**
     * @brief Check if xedd needs CALL emulation
     */
    inline bool need_emul_call(const xed_decoded_inst_t *xedd,
                               const xed_category_enum_t instr_cat,
                               const xed_iclass_enum_t instr_cls) const {
        return (instr_cat == XED_CATEGORY_CALL);
    }

    /**************************************************************************/
    /* Static functions *******************************************************/
    /**************************************************************************/

    /**
     * @brief Check if the XED instruction do RIP-rel memory access
     */
    static inline bool
    does_rip_rel_memory_access(const xed_decoded_inst_t *xedd);

    /**
     * @brief Check if the XED instruction is CALL with memory op
     */
    static inline bool is_mem_load_call_or_jmp(const xed_decoded_inst_t *xedd);

    /**
     * @brief Extract the fixed (GSRIP) portion from CA
     */
    static inline uint64_t normalize_gsrip(const uint64_t ca);

    inline uint64_t read_ra_at_rsp(const uint64_t rsp) const {
        return is_encoded_cc_ptr(rsp)
                       ? cc_->encrypt_decrypt_u64(rsp, con_->read_mem_8B(rsp))
                       : con_->read_mem_8B(rsp);
    }

    inline void write_ra_at_rsp(const uint64_t rsp,
                                const uint64_t new_ra) const {
        con_->write_mem_8B(rsp, (is_encoded_cc_ptr(rsp)
                                         ? cc_->encrypt_decrypt_u64(rsp, new_ra)
                                         : new_ra));
    }

    inline void reset_internal_state() {
        ifdbgprint((kTrace && dbg() &&
                    (need_cp_fixup_ || need_ra_fixup_ || is_rip_rel_ ||
                     reg_fixup_ != X86_Reg_Id_Not_Used)),
                   "reset_internal_state: Reset shadow_rip vars");
        // Need to do this here since exceptions may short-circuit a cycle such
        // that any of the other callbacks here may not be triggered and thus
        // leave these variables set for more than one cycle.
        need_cp_fixup_ = false;
        need_ra_fixup_ = false;
        is_rip_rel_ = false;
        reg_fixup_ = X86_Reg_Id_Not_Used;
    }
};  // class ShadowRip

template <typename CcTy, typename ConTy, typename CtxTy>
inline void ShadowRip<CcTy, ConTy, CtxTy>::register_callbacks() {
    con_->register_instruction_decoder_cb(
            [](auto, auto, auto *decoder_handle, auto *iq_handle, void *m) {
                auto shadow_rip = reinterpret_cast<SelfTy *>(m);
                return shadow_rip->instruction_decode(decoder_handle,
                                                      iq_handle);
            },
            [](auto, auto, auto, auto) -> tuple_int_string_t {
                // Simply defer to default disassembler:
                return {.integer = 0, .string = NULL};
            },
            reinterpret_cast<void *>(this));

    con_->register_instruction_before_cb(
            [](auto *, auto *, auto *instr, void *m) -> void {
                auto shadow_rip = reinterpret_cast<SelfTy *>(m);
                shadow_rip->instruction_before_cb(instr);
            },
            reinterpret_cast<void *>(this));

    con_->register_instruction_after_cb(
            [](auto *, auto *, auto *instr, void *m) -> void {
                auto shadow_rip = reinterpret_cast<SelfTy *>(m);
                shadow_rip->instruction_after_cb(instr);
            },
            reinterpret_cast<void *>(this));

    con_->register_exception_after_cb(
            CPU_Exception_All,
            [](auto *, auto *, auto *eh, auto *m) {
                auto shadow_rip = reinterpret_cast<SelfTy *>(m);
                shadow_rip->exception_after_cb(eh);
            },
            reinterpret_cast<void *>(this));

    con_->register_exception_before_cb(
            CPU_Exception_All,
            [](auto *, auto *, auto *eh, auto *m) {
                auto shadow_rip = reinterpret_cast<SelfTy *>(m);
                shadow_rip->exception_before_cb(eh);
            },
            reinterpret_cast<void *>(this));

    con_->register_smm_enter_before_cb(
            [](auto *, auto *, auto *m) {
                reinterpret_cast<SelfTy *>(m)->smm_enter_before_cb();
            },
            reinterpret_cast<void *>(this));

    con_->register_smm_leave_after_cb(
            [](auto *, auto *, auto *m) {
                reinterpret_cast<SelfTy *>(m)->smm_leave_after_cb();
            },
            reinterpret_cast<void *>(this));
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline uint64_t
ShadowRip<CcTy, ConTy, CtxTy>::address_before_shim(logical_address_t la,
                                                   address_handle_t *) {
    if (is_rip_rel_) {
        if (rip_rel_val_ != la) {
            ifdbgprint(dbg(),
                       "rip_rel_val_ != la (0x%lx != 0x%lx , rip: 0x%lx) -> "
                       "not applying GSRIP",
                       rip_rel_val_, (uint64_t)la, con_->read_rip());
            return la;
        }

        ifdbgprint(kTrace && dbg(),
                   "address_before_shim: calling "
                   "replace_shadow_rip(0x%016lx), rip is 0x%016lx",
                   (uint64_t)la, con_->read_rip());
        const uint64_t new_la = replace_shadow_rip(la);
        ifdbgprint(kTrace && dbg(), "address_before_shim: 0x%016lx -> 0x%016lx",
                   (uint64_t)la, new_la);
        return new_la;
    }
    return la;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline void ShadowRip<CcTy, ConTy, CtxTy>::instruction_after_cb(
        instruction_handle_t *handle) {
#ifndef CC_DISABLE_SHADOW_RIP_HACK
    if (need_ra_fixup_) {
        // Read stack pointer, and then load return address
        const auto rsp = con_->read_rsp();
        const auto ra = read_ra_at_rsp(rsp);

        // Add gsrip fixed bits to the RA and write back to stack
        const auto new_ra = replace_shadow_rip(ra, old_gsrip_);
        ifdbgprint(dbg(), "Replacing RA with gsrip|RA 0x%016lx", new_ra);
        write_ra_at_rsp(rsp, new_ra);
    }
#endif  // CC_DISABLE_SHADOW_RIP_HACK

    if (need_cp_fixup_) {
        ifdbgprint(dbg(), "Restoring in-mem CA at 0x%016lx to 0x%016lx",
                   original_cp_addr_, original_cp_value_);
        con_->write_mem_8B(original_cp_addr_, original_cp_value_);
        // Restore integrity enabling to prior value
        if (mem_call_integrity_hack_) {
            ifdbgprint(dbg(), "re-enabling integrity after call");
            ctx_->set_icv_enabled(true);
        }
    }

    if (reg_fixup_ != X86_Reg_Id_Not_Used) {
        con_->set_gpr(reg_fixup_, reg_fixup_val_);
        reg_fixup_ = X86_Reg_Id_Not_Used;
    }
}

template <typename CcTy, typename ConTy, typename CtxTy>
void ShadowRip<CcTy, ConTy, CtxTy>::exception_after_cb(exception_handle_t *eh) {
    // Read RSP and decode it if it is a CA (implicit by decode_pointer)
    const uint64_t rsp_reg = con_->read_rsp();
    const uint64_t rsp_decoded =
            (is_encoded_cc_ptr(rsp_reg) ? cc_->decode_pointer(rsp_reg)
                                        : rsp_reg);
    ifdbgprint(kTraceExceptionAfter, "Loaded RSP: %lx (undecoded: 0x%016lx)",
               rsp_decoded, rsp_reg);

    //  Read return address and decrypt if RSP is CA
    uint64_t ra_old = con_->read_mem_8B(rsp_decoded);
    ra_old = ((rsp_reg != rsp_decoded && !con_->disable_data_encryption)
                      ? cc_->encrypt_decrypt_u64(rsp_reg, ra_old)
                      : ra_old);

    // Put gsrip into return address
    uint64_t ra_new = replace_shadow_rip(ra_old);
    ra_new = ((rsp_reg != rsp_decoded && !con_->disable_data_encryption)
                      ? cc_->encrypt_decrypt_u64(rsp_reg, ra_new)
                      : ra_new);

    ifdbgprint(kTraceExceptionAfter,
               "Updating RA with GSRIP at %lx (0x%016lx -> 0x%016lx)",
               rsp_decoded, ra_old, ra_new);

    // And write the gsripped return address to memory
    con_->write_mem_8B(rsp_decoded, ra_new);

    // Finally, clear out current gsrip
    ifdbgprint(kTraceExceptionAfter, "Updating GSRIP from 0x%016lx -> 0x%016lx",
               get_shadow_rip(), 0LU);
    set_shadow_rip(0);
}

template <typename CcTy, typename ConTy, typename CtxTy>
void ShadowRip<CcTy, ConTy, CtxTy>::smm_enter_before_cb() {
    ifdbgprint(kTraceSMM, "Updating GSRIP from 0x%016lx -> 0x%016lx",
               get_shadow_rip(), 0LU);
    const uint64_t rip_old = con_->read_rip();
    const uint64_t rip_new = replace_shadow_rip(rip_old);

    ifdbgprint(kTraceSMM, "Storing GSRIP in IP (0x%016lx -> 0x%016lx)", rip_old,
               rip_new);

    con_->write_rip(rip_new);
    set_shadow_rip(0);
}

template <typename CcTy, typename ConTy, typename CtxTy>
void ShadowRip<CcTy, ConTy, CtxTy>::smm_leave_after_cb() {
    const uint64_t rip_old = con_->read_rip();

    const uint64_t gsrip_new =
            (is_encoded_cc_ptr(rip_old) ? normalize_gsrip(rip_old) : 0LU);
    const uint64_t rip_new =
            (is_encoded_cc_ptr(rip_old) ? cc_->decode_pointer(rip_old)
                                        : rip_old);

    ifdbgprint(kTraceSMM, "Updating GSRIP from 0x%016lx -> 0x%016lx",
               get_shadow_rip(), gsrip_new);
    ifdbgprint(kTraceSMM, "Updating IP from 0x%016lx -> 0x%016lx", rip_old,
               rip_new);

    con_->write_rip(rip_new);
    set_shadow_rip(gsrip_new);
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline int ShadowRip<CcTy, ConTy, CtxTy>::instruction_decode(
        decoder_handle_t *decoder_handle, instruction_handle_t *instr_handle) {
    if (!con_->is_64_bit_mode()) {
        return 0;
    }

    // Use unique_ptr to trigger destructor unless released
    xed_decoded_inst_t xedd;
    xed_decoded_inst_zero(&xedd);
    xed_decoded_inst_set_mode(&xedd, kXedMMode, kXedAddrWidth);

    // Decode instruction
    auto instr_bytes = con_->get_instruction_bytes(instr_handle);
    auto err = xed_decode(&xedd, instr_bytes.data, instr_bytes.size);
    if (err != XED_ERROR_NONE) {  // Ignore unrecognized instructions here
        return 0;
    }

    // Type ot pass both *this and xedd to decode callback

    const auto instr_cls = xed_decoded_inst_get_iclass(&xedd);
    const auto instr_cat = xed_decoded_inst_get_category(&xedd);

    // Emulate CALL
    if (need_emul_call(&xedd, instr_cat, instr_cls)) {
        con_->register_emulation_cb(xedd_pair_emul_callback(emul_call),
                                    decoder_handle, gen_xedd_pair(this, xedd),
                                    xedd_pair_free_lambda);
        return xed_decoded_inst_get_length(&xedd);
    }

    // Emulate JMP (ignore conditional jumps, we only care about tail calls)
    if (is_jmp_instr(instr_cls)) {
        con_->register_emulation_cb(xedd_pair_emul_callback(emul_jmp),
                                    decoder_handle, gen_xedd_pair(this, xedd),
                                    xedd_pair_free_lambda);
        return xed_decoded_inst_get_length(&xedd);
    }

    // Check if we got an LEA instruction
    if (instr_cls == XED_ICLASS_LEA) {
        // Skip unless we're dealing with RIP-relative LEA
        if (xed_decoded_inst_get_base_reg(&xedd, 0) != XED_REG_RIP) {
            return 0;
        }

        con_->register_emulation_cb(xedd_pair_emul_callback(emul_lea),
                                    decoder_handle, gen_xedd_pair(this, xedd),
                                    xedd_pair_free_lambda);
        return xed_decoded_inst_get_length(&xedd);
    }

    // Check if the instruction has memory operands
    if (does_rip_rel_memory_access(&xedd)) {
        con_->register_emulation_cb(xedd_pair_emul_callback(emul_mem_access),
                                    decoder_handle, gen_xedd_pair(this, xedd),
                                    xedd_pair_free_lambda);
        return xed_decoded_inst_get_length(&xedd);
    }

    // We have a RET
    if (need_emul_ret(&xedd, instr_cat)) {
        con_->register_emulation_cb(xedd_pair_emul_callback(emul_ret),
                                    decoder_handle, gen_xedd_pair(this, xedd),
                                    xedd_pair_free_lambda);
        return xed_decoded_inst_get_length(&xedd);
    }

    return 0;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline cpu_emulation_t
ShadowRip<CcTy, ConTy, CtxTy>::emul_lea(xed_decoded_inst_t *xedd) {
    if (!is_enabled()) {
        return CPU_Emulation_Default_Semantics;
    }

    xed_uint64_t new_dst_val;
    if (!xed_agen_or_fault(xedd, 0, this, &new_dst_val)) {
        return CPU_Emulation_Default_Semantics;
    }

    // XED_OPERAND_OUTREG dosen't seem to work here!?!
    auto dst_xed_reg = xed_decoded_inst_get_reg(xedd, XED_OPERAND_REG0);
    auto dst_reg = convert_xed_reg_to_simics(dst_xed_reg);
    if (dst_reg == X86_Reg_Id_Not_Used) {
        return CPU_Emulation_Default_Semantics;
    }

    if (dbg() && get_shadow_rip() != 0) {
        dbgprint("%-25s 0x%016lx", "gsrip is ", get_shadow_rip());
        dbgprint("%-25s 0x%016lx", "rip is ", con_->read_rip());
        dbgprint("Chaning output reg %s", convert_reg_to_string(dst_reg));
        dbgprint("%-25s 0x%016lx", "to ", new_dst_val);
    }

    con_->set_gpr(dst_reg, new_dst_val);
    return CPU_Emulation_Fall_Through;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline cpu_emulation_t
ShadowRip<CcTy, ConTy, CtxTy>::emul_mem_access(xed_decoded_inst_t *xedd) {
    if (!is_enabled()) {
        return CPU_Emulation_Default_Semantics;
    }

    // We may have multiple memory accesses, so let's get the RIP-rel address
    // and store it so we can avoid modifying others (e.g., in instructions
    // such as `push qword ptr [rip+0xae140]`).

    uint64_t val;
    if (!xed_agen_or_fault(xedd, 0, this, &val)) {
        dbgprint("XED agent failed to decode instruction at %lx",
                 con_->read_rip());
        break_or_die("XED agen failed", 1);
        return CPU_Emulation_Default_Semantics;
    }

    is_rip_rel_ = true;
    // Need the LA since that is what we're replacing in address_before_shim
    rip_rel_val_ = is_encoded_cc_ptr(val) ? cc_->decode_pointer(val) : val;
    // ifdbgprint(true, "setting rip_rel_val_ to 0x%lx (0x%lx)",
    //            rip_rel_val_, val);

    return CPU_Emulation_Default_Semantics;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline cpu_emulation_t
ShadowRip<CcTy, ConTy, CtxTy>::emul_call_or_jmp(xed_decoded_inst_t *xedd,
                                                const bool is_call) {
    if (!is_enabled()) {
        return CPU_Emulation_Default_Semantics;
    }

    ifdbgprint(kTrace && dbg(), "0x%016lx START", con_->read_rip());
    if (is_call) {
        const auto old_gsrip = get_shadow_rip();

        // Check if we need to store old_gsrip in the return address later
        if (old_gsrip != 0) {
            ifdbgprint(dbg(), "Need to save old gsrip 0x%016lx", old_gsrip);
            old_gsrip_ = old_gsrip;  // Save old gsrip for the later
            need_ra_fixup_ = true;   // after_instruction callback.
        }
    }

    // Are we dealing with an indirect call to implicitly loaded ptr (e.g.,
    // an instruction of form `call qword ptr [rdi]`)
    if (update_mem_branch_target(xedd, is_call)) {
        ifdbgprint(kTrace && dbg(), "END mem_branch");
        return CPU_Emulation_Default_Semantics;
    }

    // Check if we have a direct reg call, e.g.,  `call [rdi]`
    if (update_reg_branch_target(xedd, is_call)) {
        ifdbgprint(kTrace && dbg(), "END reg_branch");
        return CPU_Emulation_Default_Semantics;
    }

    ifdbgprint(kTrace && dbg(), "END fallthrough");
    return CPU_Emulation_Default_Semantics;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline cpu_emulation_t
ShadowRip<CcTy, ConTy, CtxTy>::emul_ret(xed_decoded_inst_t *xedd) {
    if (!is_enabled()) {
        return CPU_Emulation_Default_Semantics;
    }

    // Read stack pointer, and then load return address
    const auto rsp = con_->read_rsp();
    const auto ra = read_ra_at_rsp(rsp);

    if (is_encoded_cc_ptr(ra)) {
        // Replace the RA on stack
        const auto decoded_ra = cc_->decode_pointer(ra);
        write_ra_at_rsp(rsp, decoded_ra);

        // Restore old shadow rip from return address
        const auto new_gsrip = normalize_gsrip(ra);
        if (new_gsrip != get_shadow_rip()) {
            ifdbgprint(dbg(), "Setting gsrip to 0x%016lx", new_gsrip);
            set_shadow_rip(new_gsrip);
        }
    } else {
        if (0 != get_shadow_rip()) {
            ifdbgprint(dbg(), "Setting gsrip to 0x%016lx", 0LU);
            set_shadow_rip(0);
        }
    }

    return CPU_Emulation_Default_Semantics;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline uint64_t
ShadowRip<CcTy, ConTy, CtxTy>::emul_lea_reg_xed_cb(xed_reg_enum_t xed_reg,
                                                   xed_bool_t *err) {
    if (xed_reg == XED_REG_RIP) {
        xed_agen_rip_replace_ = true;
        return replace_shadow_rip(con_->read_rip());
    } else {
        auto reg = convert_xed_reg_or_fault(xed_reg);
        if (reg == X86_Reg_Id_Not_Used) {
            *err = XED_ERROR_BAD_REGISTER;
            return 0;
        }
        return con_->get_gpr(reg);
    }
}

/**
 * @brief This handles calls of type `jump [rax]`
 */
template <typename CcTy, typename ConTy, typename CtxTy>
inline bool ShadowRip<CcTy, ConTy, CtxTy>::update_mem_branch_target(
        xed_decoded_inst_t *xedd, const bool is_call) {
    if (!is_mem_load_call_or_jmp(xedd)) {
        return false;
    }

    uint64_t xed_ptr_address;
    if (!xed_agen_or_fault(xedd, 0, this, &xed_ptr_address)) {
        return true;
    }

    // ptr_address is the memory location of the pointer with the branch target
    const uint64_t ptr_address = xed_ptr_address;

    const bool ptr_address_is_ca = is_encoded_cc_ptr(ptr_address);
    const uint64_t ptr_address_la =
            ptr_address_is_ca ? cc_->decode_pointer(ptr_address) : ptr_address;

    if (ptr_address_is_ca) {
        ifdbgprint(dbg(), "ptr_address->decoded: 0x%016lx->0x%016lx",
                   ptr_address, ptr_address_la);
    }

    // Now let's read the actual branch target from memory
    const uint64_t cp_encrypted = con_->read_mem_8B(ptr_address_la);
    // And decrypt the branch target if the it was encrypted in memory
    const uint64_t cp_decrypted =
            (ptr_address_is_ca && !con_->disable_data_encryption)
                    ? cc_->encrypt_decrypt_u64(ptr_address, cp_encrypted)
                    : cp_encrypted;

    // Update the gsrip based on the on the branch target
    const uint64_t cp_decoded =
            (kUpdateGsripOnJMP || is_call)
                    ? decode_ptr_and_update_gsrip(cp_decrypted)
                    : decode_ptr(cp_decrypted);

    if (cp_encrypted != cp_decrypted || cp_decrypted != cp_decoded) {
        ifdbgprint(dbg(), "ptr, decr, deco 0x%016lx->0x%016lx->0x%016lx",
                   cp_encrypted, cp_decrypted, cp_decoded);
    }

    if (cp_decoded != cp_encrypted) {
        need_cp_fixup_ = true;
        original_cp_value_ = cp_encrypted;
        original_cp_addr_ = ptr_address_la;

        // Re-encrypt ptr if it accessed via CA, but not RIP-rel access.
        //
        // In the RIP-rel case, the call emulation will use the plaintext
        // RIP-rel generated LA, otherwise the input ptr already was a CA
        // and will correctly decrypt again on emulation of the call.
        uint64_t cp_new_temp = 0;

        if (ptr_address_is_ca && !xed_agen_rip_replace_) {
            // The access was not rip-rel, and the actual pointer used was a CA.
            // This indirect call is a CA mem-load, so it will get correctly
            // decrypted when the call accesses the memory to load the target.
            if (!con_->disable_data_encryption) {
                cp_new_temp = cc_->encrypt_decrypt_u64(ptr_address, cp_decoded);
            }
        } else {
            // We're going to have a plaintext access since we cannot mess with
            // the actual RIP and want to rely on regular Simics behavior to
            // handle all the heavy lifting of emulating the call.
            cp_new_temp = cp_decoded;

            if (xed_agen_rip_replace_) {
                // However, because the actual memory is C3 protected and we
                // here replace it with a plaintext call target, we will
                // potentially get an ICV violation. So, let's disable integrity
                // for the duration fo the call.
                ifdbgprint(dbg(), "disable integrity for duration of call");
                mem_call_integrity_hack_ = ctx_->get_icv_enabled();
                ctx_->set_icv_enabled(false);
            }
        }
        ifdbgprint(dbg(), "replacing in memory ptr 0x%016lx->0x%016lx",
                   cp_encrypted, cp_new_temp);
        con_->write_mem_8B(ptr_address_la, cp_new_temp);
    }

    return true;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline bool ShadowRip<CcTy, ConTy, CtxTy>::update_reg_branch_target(
        xed_decoded_inst_t *xedd, const bool is_call) {
    const auto xed_reg = xed_decoded_inst_get_reg(xedd, XED_OPERAND_REG0);
    if (xed_reg == XED_REG_STACKPUSH || (!is_call && xed_reg == XED_REG_RIP)) {
        return false;
    }

    // Get the call target so we can check if it is a CA
    const auto reg = convert_xed_reg_to_simics(xed_reg);
    const auto call_target = con_->get_gpr(reg);

    ifdbgprint(kTrace && dbg(), "Indirect branch to 0x%016lx", call_target);

    const uint64_t decoded = (kUpdateGsripOnJMP || is_call)
                                     ? decode_ptr_and_update_gsrip(call_target)
                                     : decode_ptr(call_target);

    if (decoded != call_target) {
        ifdbgprint(dbg(), "Replacing: 0x%016lx -> 0x%016lx", call_target,
                   decoded);
        // Since the register could be callee-saved, we need to make sure we
        // restore the old value after the instruction!
        reg_fixup_ = reg;
        reg_fixup_val_ = call_target;
        con_->set_gpr(reg, decoded);
    }

    return true;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline uint64_t ShadowRip<CcTy, ConTy, CtxTy>::decode_ptr_and_update_gsrip(
        const uint64_t call_target, bool do_gsrip_update) {
    const bool is_ca = is_encoded_cc_ptr(call_target);

    if (do_gsrip_update) {
        // Then get our new gsrip depending on whether target was CA
        const auto new_gsrip = is_ca ? normalize_gsrip(call_target) : 0;
        const auto old_gsrip = get_shadow_rip();

        // Finally, update the gsrip if it has changed
        if (new_gsrip != old_gsrip) {
            ifdbgprint(dbg(), "Setting gsrip to 0x%016lx", new_gsrip);
            set_shadow_rip(new_gsrip);  // Set new shadow-rip.
        }
    }

    if (is_ca) {
        // Decode call target if it is a CA
        const auto decoded = cc_->decode_pointer(call_target);
        ifdbgprint(dbg(), "Decoding call/jmp target: 0x%016lx -> 0x%016lx",
                   call_target, (uint64_t)decoded);
        return decoded;
    } else {
        return call_target;
    }
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline uint64_t
ShadowRip<CcTy, ConTy, CtxTy>::replace_shadow_rip(const uint64_t la,
                                                  const uint64_t gsrip) {
    // Get decoded shadow-rip
    const auto la_gsrip = get_decoded_shadow_rip(gsrip);

    // Generate mask for mutable pointer bits
    const auto ptr_md = get_pointer_metadata(gsrip);
    const auto mask = get_tweak_mask(ptr_md.size_);

    // Make sure our address is within the shadow-rip range
    const auto fixed_la_gsrip = mask & la_gsrip;
    const auto fixed_la = mask & la;
    if (fixed_la_gsrip != fixed_la) {
        if (dbg() || kBreakOnGsripEscape || kFaultOnGsripEscape) {
            dbgprint("gsrip is       0x%016lx", gsrip);
            dbgprint("rip is         0x%016lx", con_->read_rip());
            dbgprint("LA   is        0x%016lx", la);
            dbgprint("Mask is        0x%016lx", mask);
            dbgprint("Rip-rel access 0x%016lx not in gsrip power slot "
                     "0x%016lx != 0x%016lx",
                     la, fixed_la_gsrip, fixed_la);
            if (kBreakOnGsripEscape) {
                SIM_break_simulation("Break on GSRIP escape\n");
            }
            if (kFaultOnGsripEscape) {
                // Disable gsrip here since we're triggering an exception
                // and do not yet handle GSRIP updates on exceptions, possibly
                // leading to executing exception handlers with an incorrect
                // gsrip set and enabled.
                //
                // NOTE: Other exceptions may still cause issues until
                //       exceptions are handled properly!
                //
                this->ctx_->set_gsrip_enabled(false);
                this->con_->gp_fault(0, false, "Fault on GSRIP escape\n");
            }
        }
        return la;  // LA is outside shadow-rip power slot
    }

    auto new_la = la ^ fixed_la;       // Remove fixed bits from address
    new_la = new_la | (mask & gsrip);  // Replace with gsrip fixed bits
    ifdbgprint(dbg(), "Applying gsrip to 0x%016lx -> 0x%016lx", la, new_la);
    return new_la;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline uint64_t
ShadowRip<CcTy, ConTy, CtxTy>::replace_shadow_rip(const uint64_t la) {
    const auto gsrip = get_shadow_rip();

    if (is_enabled() && gsrip != 0) {
        ASSERT_FMT(is_encoded_cc_ptr(gsrip), "GSRIP not a CA: %016lx ", gsrip);
        return replace_shadow_rip(la, gsrip);
    }

    return la;  // Return pointer as-is if GSRIP disabled or not set
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline bool
ShadowRip<CcTy, ConTy, CtxTy>::xed_agen_or_fault(xed_decoded_inst_t *xedd,
                                                 uint64_t memop_index,
                                                 void *context, uint64_t *dst) {
    xed_agen_register_callback(
            [](auto xed_reg, void *data, auto *err) -> xed_uint64_t {
                auto sr = reinterpret_cast<SelfTy *>(data);
                return sr->emul_lea_reg_xed_cb(xed_reg, err);
            },
            [](auto, auto, auto) -> xed_uint64_t { return 0; });

    xed_agen_rip_replace_ = false;
    auto err = xed_agen(xedd, memop_index, context, dst);
    if (err == XED_ERROR_NONE) {
        return true;
    }
    *dst = 0;
    dbgprint("xed_agen failed to get addr");
    fault_and_break_if_enabled("xed agen_failed");
    return false;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline int ShadowRip<CcTy, ConTy, CtxTy>::convert_xed_reg_or_fault(
        xed_reg_enum_t xed_reg) {
    auto simics_reg = convert_xed_reg_to_simics(xed_reg);
    if (simics_reg == X86_Reg_Id_Not_Used) {
        dbgprint("xed_reg->simics_reg %d failed", xed_reg);
        fault_and_break_if_enabled("xed->simics reg conversion failed");
    }
    return simics_reg;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline void ShadowRip<CcTy, ConTy, CtxTy>::set_shadow_rip(uint64_t new_gsrip) {
    if (kBreakOnGsripChange) {
        const auto old_gsrip = get_shadow_rip();
        if (old_gsrip != new_gsrip) {
            dbgprint("Set gsrip 0x%016lx -> 0x%016lx and break", old_gsrip,
                     new_gsrip);
            SIM_break_simulation("Break on gsrip change "
                                 "(kBreakOnGsripChange=true)");
        }
    }
    ctx_->set_gsrip(new_gsrip);
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline bool ShadowRip<CcTy, ConTy, CtxTy>::does_rip_rel_memory_access(
        const xed_decoded_inst_t *xedd) {
    const unsigned int noperands = xed_decoded_inst_noperands(xedd);
    for (unsigned i = 0; i < noperands; ++i) {
        if (xed_decoded_inst_get_base_reg(xedd, 0) == XED_REG_RIP) {
            if (xed_decoded_inst_mem_read(xedd, i))
                return true;
            if (xed_decoded_inst_mem_written(xedd, i))
                return true;
        }
    }
    return false;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline bool ShadowRip<CcTy, ConTy, CtxTy>::is_mem_load_call_or_jmp(
        const xed_decoded_inst_t *xedd) {
    const auto iform = xed_decoded_inst_get_iform_enum(xedd);
    return (iform == XED_IFORM_JMP_MEMv || iform == XED_IFORM_JMP_FAR_MEMp2 ||
            iform == XED_IFORM_CALL_NEAR_MEMv ||
            iform == XED_IFORM_CALL_NEAR_MEMv ||
            iform == XED_IFORM_CALL_FAR_MEMp2);
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline uint64_t
ShadowRip<CcTy, ConTy, CtxTy>::normalize_gsrip(const uint64_t ca) {
    const auto md = get_pointer_metadata(ca);
    const auto mask = get_tweak_mask(md.size_);
    return (mask & ca);
}

}  // namespace ccsimics

#endif  // MODULES_COMMON_CCSIMICS_SHADOW_RIP_H_
