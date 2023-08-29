/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_COMMON_CCSIMICS_INTEGRITY_ISA_H_
#define MODULES_COMMON_CCSIMICS_INTEGRITY_ISA_H_

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

/**
 * @brief Implements the ISA extensions for Integrity checking
 * @tparam ConTy
 * @tparam PETy
 */
template <typename ConTy, typename PETy> class IntegrityIsa {
    using SelfTy = IntegrityIsa<ConTy, PETy>;

 public:
    static constexpr uint8_t kLockInstByte0 = 0xF0;  // LOCK prefix

    static constexpr uint8_t kInvICVInstByte1 = 0x48;
    static constexpr uint8_t kInvICVInstByte2 = 0x2B;
    static constexpr uint8_t kInvICVInstByte3 = 0xc0;

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

    inline int decode(decoder_handle_t *handle,
                      instruction_handle_t *iq_handle) {
        cpu_bytes_t bytes = m_con_->get_instruction_bytes(iq_handle);

        if (bytes.size == 0) {
            return -1;
        }
        if (bytes.data[0] != kLockInstByte0) {
            return 0;
        }

        if (bytes.size < 2) {
            return -2;
        }
        if (bytes.size < 3) {
            return -3;
        }
        if (bytes.data[2] != kInvICVInstByte2) {
            return 0;
        }

        if (bytes.size < 4) {
            return -4;
        }

        if (bytes.data[3] == kInvICVInstByte3) {
            icv_emul_data_t *cbdata = MM_ZALLOC(1, icv_emul_data_t);

            // Store the register op numbers in cbdata
            auto res = xed_decode_register(bytes.data, 4, &cbdata->regs_);
            ASSERT_FMT(res >= 0, "failed to get regs from %02x%02x%02x%02x",
                       bytes.data[0], bytes.data[1], bytes.data[2],
                       bytes.data[3]);
            // Store pointer to self in cbdata
            cbdata->self_ = static_cast<void *>(this);

            const int dest_ptr_reg = cbdata->regs_.dest_ptr_reg_;
            const int src_ptr_reg = cbdata->regs_.src_ptr_reg_;

            if (bytes.data[3] == kInvICVInstByte3 &&
                dest_ptr_reg == src_ptr_reg) {
                ifdbgprint(is_debug(),
                           "detected INVICV at %lx "
                           "with opcode: %02x %02x %02x %02x (regs: %d, %d)\n",
                           m_con_->read_rip(), bytes.data[0], bytes.data[1],
                           bytes.data[2], bytes.data[3], dest_ptr_reg,
                           src_ptr_reg);

                m_con_->register_emulation_cb(
                        [](auto *obj, auto *cpu, void *cbdata) {
                            auto *emul_data =
                                    static_cast<icv_emul_data_t *>(cbdata);
                            auto *self =
                                    static_cast<SelfTy *>(emul_data->self_);
                            return self->cc_invicv_emulation(&emul_data->regs_);
                        },
                        handle, cbdata,
                        [](auto *obj, auto *cpu, auto *data) {
                            MM_FREE(data);
                        });
            } else {
                // ERROR
                ifdbgprint(is_debug(),
                           "ERROR detected ccINVICV at %lx "
                           "with opcode: %02x %02x %02x %02x (regs: %d, %d)\n",
                           m_con_->read_rip(), bytes.data[0], bytes.data[1],
                           bytes.data[2], bytes.data[3], dest_ptr_reg,
                           src_ptr_reg);
            }
            return 4;
        }
        return 0;
    }

    inline cpu_emulation_t cc_invicv_emulation(void *cbdata) {
        const auto *regs = static_cast<cc_icv_regs_t *>(cbdata);
        const int dest_ptr_reg = regs->dest_ptr_reg_;
        const int src_ptr_reg = regs->src_ptr_reg_;

        uint64_t dest_ptr = m_con_->get_gpr(dest_ptr_reg);
        // uint64_t src_ptr = m_con_->get_gpr(src_ptr_reg);

        auto *integrity = m_pe_->get_integrity();

        if (integrity != nullptr && is_enabled()) {
            integrity->invalidateICV(dest_ptr);

            ifdbgprint(is_debug(),
                       "INVICV at 0x%016lx operands %d, %d:\n"
                       "       input ptr  <- 0x%016lx\n",
                       m_con_->read_rip(), dest_ptr_reg, src_ptr_reg, dest_ptr);
        } else {
            // TODO error
        }

        return CPU_Emulation_Fall_Through;
    }

    static inline tuple_int_string_t disassemble(generic_address_t addr,
                                                 cpu_bytes_t bytes) {
        if (bytes.size == 0) {
            return {-1, NULL};
        }
        if (bytes.data[0] != kLockInstByte0) {
            return {0, NULL};
        }

        if (bytes.size < 2) {
            return {-2, NULL};
        }
        if (bytes.size < 3) {
            return {-3, NULL};
        }
        if (bytes.data[2] != kInvICVInstByte2) {
            return {0, NULL};
        }

        if (bytes.size < 4) {
            return {-4, NULL};
        }

        if (bytes.data[3] == kInvICVInstByte3) {
            cc_icv_regs_t *cc_isa_regs = MM_ZALLOC(1, cc_icv_regs_t);
            xed_decode_register(bytes.data, 4, cc_isa_regs);
            return {4, MM_STRDUP("invicv")};
        }

        return {0, NULL};
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

#endif  // MODULES_COMMON_CCSIMICS_INTEGRITY_ISA_H_
