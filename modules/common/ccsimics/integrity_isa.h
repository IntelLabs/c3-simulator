/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_COMMON_CCSIMICS_INTEGRITY_ISA_H_
#define MODULES_COMMON_CCSIMICS_INTEGRITY_ISA_H_

#include <utility>
extern "C" {
#include <xed-interface.h>  // NOTE! The xed header is C-only
}
#include "ccsimics/data_encryption.h"
#include "ccsimics/xed_util.h"
#include "malloc/cc_globals.h"

#define LOCK_OFFSET 1

namespace ccsimics {

typedef struct {
    int dest_ptr_reg_;
    int src_ptr_reg_;
} cc_icv_regs_t;

typedef struct {
    void *self_;
    cc_icv_regs_t regs_;
} icv_emul_data_t;

#define _dump_decoded_inst(name, data, regs)                                   \
    do {                                                                       \
        dbgprint("found %s at 0x%lx (inst: %02x%02x%02x%02x regs: %d, %d)",    \
                 (name), m_con_->read_rip(), (data)[0], (data)[1], (data)[2],  \
                 (data)[3], (regs.dest_ptr_reg_), (regs.src_ptr_reg_));        \
    } while (0);

/**
 * @brief Implements the ISA extensions for Integrity checking
 * @tparam ConTy
 * @tparam PETy
 */
template <typename ConTy, typename PETy> class IntegrityIsa {
    using SelfTy = IntegrityIsa<ConTy, PETy>;

    enum IntegrityInst { INVICV, INITICV, PREINITICV, OTHER };

 public:
    static constexpr uint8_t kLockInstByte0 = 0xF0;  // LOCK prefix

    static constexpr uint8_t kInvICVInstByte1 = 0x48;
    static constexpr uint8_t kInvICVInstByte2 = 0x2B;
    static constexpr uint8_t kInvICVInstByte3 = 0xc0;

    static constexpr uint8_t kInitICVInstByte1 = 0x48;
    static constexpr uint8_t kInitICVInstByte2 = 0x21;

    static constexpr uint8_t kInstByte3Min = 0xc0;

 protected:
    ConTy *const m_con_;
    PETy *const m_pe_;

 public:
    inline IntegrityIsa(ConTy *con, PETy *pe) : m_con_(con), m_pe_(pe) {
        xed_tables_init();
    }

    virtual ~IntegrityIsa() {}
    bool is_debug() { return m_con_->debug_on; }
    bool is_enabled() { return m_con_->integrity; }
    /**
     * @brief Call once to register illegal_instruction_cb for Simics
     *
     * @param conf
     */
    inline void register_callbacks(conf_object_t *conf) {
        m_con_->register_illegal_instruction_cb(
                conf,
                [](auto *obj, auto *cpu, auto *handle, auto *iq_handle,
                   auto data) {
                    return static_cast<SelfTy *>(data)->decode(handle,
                                                               iq_handle);
                },
                [](auto *obj, auto *cpu, auto addr, auto bytes) {
                    return SelfTy::disassemble(addr, bytes);
                },
                static_cast<void *>(this));
    }

    static inline std::pair<enum IntegrityInst, int32_t>
    decode_cpu_bytes(cpu_bytes_t *bytes, cc_icv_regs_t *regs) {
        if (bytes->size == 0) {
            return std::make_pair(OTHER, -1);
        }

        if (bytes->data[0] != kLockInstByte0) {
            return std::make_pair(OTHER, 0);
        }

        if (bytes->size < 2) {
            return std::make_pair(OTHER, -2);
        }

        if (bytes->size < 3) {
            return std::make_pair(OTHER, -3);
        }

        if (bytes->data[2] != kInvICVInstByte2 &&
            bytes->data[2] != kInitICVInstByte2) {
            return std::make_pair(OTHER, 0);
        }

        if (bytes->size < 4) {
            return std::make_pair(OTHER, -4);
        }

        if (bytes->data[2] == kInvICVInstByte2 &&
            bytes->data[3] == kInvICVInstByte3) {
            auto res = xed_decode_register(bytes->data, 4, regs);
            ASSERT_FMT(res >= 0, "failed to get regs from %02x%02x%02x%02x",
                       bytes->data[0], bytes->data[1], bytes->data[2],
                       bytes->data[3]);

            if (regs->src_ptr_reg_ == regs->dest_ptr_reg_) {
                return std::make_pair(INVICV, 4);
            }
        }

        if (bytes->data[2] == kInitICVInstByte2 &&
            bytes->data[3] >= kInstByte3Min) {
            auto res = xed_decode_register(bytes->data, 4, regs);
            ASSERT_FMT(res >= 0, "failed to get regs from %02x%02x%02x%02x",
                       bytes->data[0], bytes->data[1], bytes->data[2],
                       bytes->data[3]);

            if (regs->src_ptr_reg_ != regs->dest_ptr_reg_) {
                return std::make_pair(INITICV, 4);
            } else {
                return std::make_pair(PREINITICV, 4);
            }
        }

        return std::make_pair(OTHER, 0);
    }

    inline icv_emul_data_t *gen_cbdata(cc_icv_regs_t *regs) {
        icv_emul_data_t *cbdata = MM_ZALLOC(1, icv_emul_data_t);
        cbdata->self_ = static_cast<void *>(this);
        cbdata->regs_.src_ptr_reg_ = regs->src_ptr_reg_;
        cbdata->regs_.dest_ptr_reg_ = regs->dest_ptr_reg_;
        return cbdata;
    }

    inline int decode(decoder_handle_t *handle,
                      instruction_handle_t *iq_handle) {
        cpu_bytes_t bytes = m_con_->get_instruction_bytes(iq_handle);
        cc_icv_regs_t regs = {0};

        auto res = decode_cpu_bytes(&bytes, &regs);

        switch (res.first) {
        case INVICV: {
            if (is_debug())
                _dump_decoded_inst("InvICV", bytes.data, regs);

            m_con_->register_emulation_cb(
                    [](auto *obj, auto *cpu, void *cbdata) {
                        auto *emul_data =
                                static_cast<icv_emul_data_t *>(cbdata);
                        auto *self = static_cast<SelfTy *>(emul_data->self_);
                        return self->cc_invicv_emulation(&emul_data->regs_);
                    },
                    handle, gen_cbdata(&regs),
                    [](auto *obj, auto *cpu, auto *data) { MM_FREE(data); });
            break;
        }
        case INITICV: {
            if (is_debug())
                _dump_decoded_inst("InitICV", bytes.data, regs);

            m_con_->register_emulation_cb(
                    [](auto *, auto *, void *d) {
                        auto *ed = static_cast<icv_emul_data_t *>(d);
                        auto *self = static_cast<SelfTy *>(ed->self_);
                        return self->emul_initicv(&ed->regs_);
                    },
                    handle, gen_cbdata(&regs),
                    [](auto *, auto *, auto *d) { MM_FREE(d); });
            break;
        }
        case PREINITICV: {
            if (is_debug())
                _dump_decoded_inst("PreInitICV", bytes.data, regs);

            m_con_->register_emulation_cb(
                    [](auto *, auto *, void *d) {
                        auto *ed = static_cast<icv_emul_data_t *>(d);
                        auto *self = static_cast<SelfTy *>(ed->self_);
                        return self->emul_preiniticv(&ed->regs_);
                    },
                    handle, gen_cbdata(&regs),
                    [](auto *, auto *, auto *d) { MM_FREE(d); });
            break;
        }
        default:
            break;
        }

        return res.second;
    }

    inline cpu_emulation_t cc_invicv_emulation(void *cbdata) {
        const auto *regs = static_cast<cc_icv_regs_t *>(cbdata);
        const int dest_ptr_reg = regs->dest_ptr_reg_;
        const int src_ptr_reg = regs->src_ptr_reg_;

        uint64_t dest_ptr = m_con_->get_gpr(dest_ptr_reg);
        // uint64_t src_ptr = m_con_->get_gpr(src_ptr_reg);

        auto *integrity = m_pe_->get_integrity();

        if (integrity != nullptr && is_enabled()) {
            integrity->invalidate_icv(dest_ptr);

            ifdbgprint(is_debug(),
                       "INVICV at 0x%016lx operands %d, %d:\n"
                       "       input ptr  <- 0x%016lx\n",
                       m_con_->read_rip(), dest_ptr_reg, src_ptr_reg, dest_ptr);
        }

        return CPU_Emulation_Fall_Through;
    }

    inline cpu_emulation_t emul_initicv(void *cbdata) {
        const auto *regs = static_cast<cc_icv_regs_t *>(cbdata);
        const int dest_ptr_reg = regs->dest_ptr_reg_;
        const int val_ptr_reg = regs->src_ptr_reg_;

        uint64_t dest_ptr = m_con_->get_gpr(dest_ptr_reg);
        uint64_t val = m_con_->get_gpr(val_ptr_reg);

        auto *integrity = m_pe_->get_integrity();

        if (integrity != nullptr && is_enabled()) {
            integrity->initialize_icv(dest_ptr, val);

            ifdbgprint(is_debug(),
                       "INITICV at 0x%016lx operands %d, %d:\n"
                       "       input ptr  <- 0x%016lx, %lu\n",
                       m_con_->read_rip(), dest_ptr_reg, val_ptr_reg, dest_ptr,
                       val);
        }

        return CPU_Emulation_Fall_Through;
    }

    inline cpu_emulation_t emul_preiniticv(void *cbdata) {
        const auto *regs = static_cast<cc_icv_regs_t *>(cbdata);
        const int dest_ptr_reg = regs->dest_ptr_reg_;
        const int src_ptr_reg = regs->src_ptr_reg_;

        uint64_t dest_ptr = m_con_->get_gpr(dest_ptr_reg);
        uint64_t src_ptr = m_con_->get_gpr(src_ptr_reg);

        auto *integrity = m_pe_->get_integrity();

        if (integrity != nullptr && is_enabled()) {
            ifdbgprint(
                is_debug(),
                "PreInitICV at 0x%016lx operands %d, %d:\n"
                "       dest ptr  <- 0x%016lx\n"
                "        src ptr  <- 0x%016lx\n",
                m_con_->read_rip(), dest_ptr_reg, src_ptr_reg, dest_ptr, src_ptr
            );

            integrity->preInitICV(dest_ptr);
        }

        return CPU_Emulation_Fall_Through;
    }

    static inline tuple_int_string_t disassemble(generic_address_t addr,
                                                 cpu_bytes_t bytes) {
        cc_icv_regs_t regs = {0};
        auto res = decode_cpu_bytes(&bytes, &regs);

        switch (res.first) {
        case INVICV: {
            return {res.second, MM_STRDUP("invicv")};
            break;
        }
        case INITICV: {
            return {res.second, MM_STRDUP("initicv")};
            break;
        }
        case PREINITICV: {
            return {res.second, MM_STRDUP("preiniticv")};
            break;
        }
        default:
            break;
        }
        return {res.second, NULL};
    }

    static inline int xed_decode_register(const uint8_t *opcode, int size,
                                          cc_icv_regs_t *cc_invicv_regs) {
        // xed3_operand_set_mpxmode(&xedd, 1);
        xed_machine_mode_enum_t mmode = XED_MACHINE_MODE_LONG_64;
        xed_address_width_enum_t stack_addr_width = XED_ADDRESS_WIDTH_64b;
        xed_decoded_inst_t xedd;
        xed_decoded_inst_zero(&xedd);
        xed_decoded_inst_set_mode(&xedd, mmode, stack_addr_width);
        xed_error_enum_t xed_error =
                xed_decode(&xedd, &opcode[LOCK_OFFSET], size - LOCK_OFFSET);
        if (xed_error != XED_ERROR_NONE) {
            ASSERT_FMT(false, "xed error: %s", xed_error_enum_t2str(xed_error));
            return -2;
        }
        xed_reg_enum_t reg0 = xed_decoded_inst_get_reg(&xedd, XED_OPERAND_REG0);
        xed_reg_enum_t reg1 = xed_decoded_inst_get_reg(&xedd, XED_OPERAND_REG1);

        cc_invicv_regs->dest_ptr_reg_ = convert_xed_reg_to_simics(reg0);
        cc_invicv_regs->src_ptr_reg_ = convert_xed_reg_to_simics(reg1);
        if (cc_invicv_regs->dest_ptr_reg_ == -1 ||
            cc_invicv_regs->src_ptr_reg_ == -1) {
            ASSERT_MSG(false, "failed to convert XED registers");
            return -1;
        }
        return 0;
    }
};

}  // namespace ccsimics

#undef _dump_decoded_inst
#endif  // MODULES_COMMON_CCSIMICS_INTEGRITY_ISA_H_
