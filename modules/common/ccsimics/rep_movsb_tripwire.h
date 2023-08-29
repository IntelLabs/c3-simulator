/*
 Copyright Intel Corporation
 SPDX-License-Identifier: MIT
*/

#ifndef MODULES_COMMON_CCSIMICS_REP_MOVSB_TRIPWIRE_H_
#define MODULES_COMMON_CCSIMICS_REP_MOVSB_TRIPWIRE_H_

#include <memory>
#include <string>
#include <tuple>
#include <utility>
extern "C" {
#include <xed-interface.h>  // NOTE! The xed header is C-only
}
#include "ccsimics/simics_connection.h"
#include "ccsimics/simics_util.h"
#include "ccsimics/xed_util.h"

#define DS_PREFIX 0x3e  // Data Segment instruction prefix
#define ES_PREFIX 0x26  // Extra Segment instruction prefix

namespace ccsimics {

/**
 * @brief REP MOVSB Tripwire copy code
 *
 * Implements functionality to copy over memory with tripwires preserved
 * using REP prefix for the MOVSB (DS-prefixed) instruction
 * Instruction: 0x3e 0xf3 0xa4
 * Src CA goes into RSI, dst CA into RDI, and number of bytes into RCX
 *
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

template <typename CcTy, typename ConTy, typename CtxTy>
class RepMovsTripwire final {
    using SelfTy = RepMovsTripwire<CcTy, ConTy, CtxTy>;
    using xedd_pair = std::pair<SelfTy *, xed_decoded_inst_t *>;

    // Unconditionally enable debug messages for this class
    static constexpr bool kDebugAlways = false;

    // Extensive output on callback entry and such
    static constexpr bool kTrace = false;

    // Break simulation on rep movs with DS  prefix
    static constexpr bool kBreakOnDsRepMovs = false;

    static constexpr bool kAlwaysEnabled = false;
    static constexpr bool kStartIn64BitMode = true;
    static constexpr bool kEnableOnStart = false;

 public:
    static const xed_machine_mode_enum_t kXedMMode = XED_MACHINE_MODE_LONG_64;
    static const xed_address_width_enum_t kXedAddrWidth = XED_ADDRESS_WIDTH_64b;

 private:
    CcTy *cc_;
    ConTy *con_;
    CtxTy *ctx_;

    bool is_64_bit_mode_ = kStartIn64BitMode;
    bool reenable_integrity_ = false;
    bool fixup_dst_icvs_ = false;
    std::tuple<uint64_t, uint64_t, uint64_t, bool>
            fixup_dst_icvs_vals_;  // (dst,src,n,DF)

 public:
    explicit RepMovsTripwire(CcTy *cc, ConTy *con, CtxTy *ctx)
        : cc_(cc), con_(con), ctx_(ctx) {}

    inline auto is_enabled() {
        return kAlwaysEnabled || ctx_->get_gsrip_enabled();
    }

    /**
     * @brief Register Simics callbacks
     */
    inline void register_callbacks();

    /**
     * @brief Register Simics callback to happen after instruction
     */
    void instruction_after_cb(instruction_handle_t *handle);

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
     * @brief Simics DS REP MOVS callback
     *
     */
    inline cpu_emulation_t emul_ds_rep_movs(xed_decoded_inst_t *xedd);

    /**
     * @brief Simics ES REP MOVS callback
     *
     */
    inline cpu_emulation_t emul_es_rep_movs(xed_decoded_inst_t *xedd);

    /**
     * @brief Simics Segment prefix REP MOVS callback
     *
     */
    inline cpu_emulation_t emul_prefix_rep_movs(xed_decoded_inst_t *xedd,
                                                INTEGRITY_SUPPRESS_MODE mode);

    inline bool dbg() { return kDebugAlways || this->con_->debug_on; }

    /**
     * @brief Raise a fault with stack trace, and fault if break_on_decode_fault
     */
    inline void fault_and_break_if_enabled(const char *msg) const {
        con_->raise_fault_with_stacktrace(msg, con_->break_on_decode_fault);
    }

    /**************************************************************************/
    /* Static functions *******************************************************/
    /**************************************************************************/
    static inline bool does_rep_movsb(const xed_decoded_inst_t *xedd,
                                      unsigned char prefix);
};  // class RepMovsTripwire

template <typename CcTy, typename ConTy, typename CtxTy>
inline void RepMovsTripwire<CcTy, ConTy, CtxTy>::register_callbacks() {
    con_->register_instruction_decoder_cb(
            [](auto, auto, auto *decoder_handle, auto *iq_handle, void *m) {
                auto rep_movsb_tripwire = reinterpret_cast<SelfTy *>(m);
                return rep_movsb_tripwire->instruction_decode(decoder_handle,
                                                              iq_handle);
            },
            [](auto, auto, auto, auto) -> tuple_int_string_t {
                // Simply defer to default disassembler:
                return {.integer = 0, .string = NULL};
            },
            reinterpret_cast<void *>(this));

    con_->register_instruction_after_cb(
            [](auto *, auto *, auto *instr, void *m) -> void {
                auto rep_movsb_tripwire = reinterpret_cast<SelfTy *>(m);
                rep_movsb_tripwire->instruction_after_cb(instr);
            },
            reinterpret_cast<void *>(this));
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline void RepMovsTripwire<CcTy, ConTy, CtxTy>::instruction_after_cb(
        instruction_handle_t *handle) {
    if (fixup_dst_icvs_) {
        ifdbgprint(dbg(), "Fixup destination ICVs after REP MOVS");
        auto integrity = cc_->get_integrity();
        // Copy over ICV values as specified in stored fixup_dst_icvs_vals_
        // We can't read the registers here because they have already been
        // modified by REP MOVSB
        auto rdi = std::get<0>(fixup_dst_icvs_vals_);
        auto rsi = std::get<1>(fixup_dst_icvs_vals_);
        auto n = std::get<2>(fixup_dst_icvs_vals_);
        auto df_flag = std::get<3>(fixup_dst_icvs_vals_);

        integrity->copyICVs(rdi, rsi, n, df_flag);
        fixup_dst_icvs_ = false;
        cc_->set_integrity_supress_mode(INTEGRITY_SUPPRESS_MODE::NONE);
    }
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline int RepMovsTripwire<CcTy, ConTy, CtxTy>::instruction_decode(
        decoder_handle_t *decoder_handle, instruction_handle_t *instr_handle) {
    if (!is_64_bit_mode_) {
        return 0;
    }

    xed_decoded_inst_t xedd;
    xed_decoded_inst_zero(&xedd);
    xed_decoded_inst_set_mode(&xedd, kXedMMode, kXedAddrWidth);

    // Decode instruction
    auto instr_bytes = con_->get_instruction_bytes(instr_handle);
    auto err = xed_decode(&xedd, instr_bytes.data, instr_bytes.size);
    if (err != XED_ERROR_NONE) {  // Ignore unrecognized instructions here
        return 0;
    }

    if (does_rep_movsb(&xedd, DS_PREFIX)) {
        ifdbgprint(dbg(), "Encountered DS REP MOVSB");
        con_->register_emulation_cb(xedd_pair_emul_callback(emul_ds_rep_movs),
                                    decoder_handle, gen_xedd_pair(this, xedd),
                                    xedd_pair_free_lambda);
        return xed_decoded_inst_get_length(&xedd);
    } else if (does_rep_movsb(&xedd, ES_PREFIX)) {
        ifdbgprint(dbg(), "Encountered ES REP MOVSB");
        con_->register_emulation_cb(xedd_pair_emul_callback(emul_es_rep_movs),
                                    decoder_handle, gen_xedd_pair(this, xedd),
                                    xedd_pair_free_lambda);
        return xed_decoded_inst_get_length(&xedd);
    }

    return 0;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline cpu_emulation_t RepMovsTripwire<CcTy, ConTy, CtxTy>::emul_ds_rep_movs(
        xed_decoded_inst_t *xedd) {
    return emul_prefix_rep_movs(xedd, INTEGRITY_SUPPRESS_MODE::STRICT);
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline cpu_emulation_t RepMovsTripwire<CcTy, ConTy, CtxTy>::emul_es_rep_movs(
        xed_decoded_inst_t *xedd) {
    return emul_prefix_rep_movs(xedd, INTEGRITY_SUPPRESS_MODE::PERMISSIVE);
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline cpu_emulation_t
RepMovsTripwire<CcTy, ConTy, CtxTy>::emul_prefix_rep_movs(
        xed_decoded_inst_t *xedd, INTEGRITY_SUPPRESS_MODE mode) {
    if (ctx_->get_icv_enabled()) {
        auto rcx = con_->get_gpr(X86_Reg_Id_Rcx);
        auto rsi = con_->get_gpr(X86_Reg_Id_Rsi);
        auto rdi = con_->get_gpr(X86_Reg_Id_Rdi);
        auto eflags = con_->read_eflags();
        auto df_flag = (eflags & 0x0400) ? 1 : 0;

        if (dbg()) {
            dbgprint("ICV rdi %lx", rdi);
            dbgprint("ICV rsi %lx", rsi);
            dbgprint("ICV rcx %lx", rcx);
            dbgprint("EFLAGS %lx", eflags);
            dbgprint("DF %x", df_flag);
        }
        fixup_dst_icvs_ = true;
        // Store the register values so we can fixup the ICV map
        // based on these values
        fixup_dst_icvs_vals_ = std::make_tuple(rdi, rsi, rcx, df_flag);
        cc_->set_integrity_supress_mode(mode);
    }
    return CPU_Emulation_Default_Semantics;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline bool RepMovsTripwire<CcTy, ConTy, CtxTy>::does_rep_movsb(
        const xed_decoded_inst_t *xedd, unsigned char prefix) {
    const auto instr_cls = xed_decoded_inst_get_iclass(xedd);

    // REP MOVSB with a 3e DS prefix
    auto inst_prefix = xed_decoded_inst_get_byte(xedd, 0);
    if (instr_cls == XED_ICLASS_REP_MOVSB && (inst_prefix == prefix)) {
        return true;
    }
    return false;
}

}  // namespace ccsimics

#endif  // MODULES_COMMON_CCSIMICS_REP_MOVSB_TRIPWIRE_H_
