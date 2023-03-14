/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_COMMON_CCSIMICS_PTRENCDEC_ISA_H_
#define MODULES_COMMON_CCSIMICS_PTRENCDEC_ISA_H_

extern "C" {
#include <xed-interface.h>  // NOTE! The xed header is C-only
}
#include "ccsimics/data_encryption.h"
#include "malloc/cc_globals.h"

#define LOCK_OFFSET 1

namespace ccsimics {

typedef struct {
    int pointer_reg_;
    int size_reg_;
} cc_ptrenc_regs_t;

typedef struct {
    void *self_;
    cc_ptrenc_regs_t regs_;
} emul_data_t;

typedef struct OpMetadata {
    union {
        uint64_t uint64_;
        struct {
            uint64_t size_ : 63;
            uint64_t is_shared_ : 1;
        };
    };
} op_metadata_t;

class DefaultCpuTy;
class DefaultPointerEncoder;

/**
 * @brief Implements the ISA extensions for encoding / decoding pointers
 *
 * @tparam CpuTy
 * @tparam PETy
 */
template <typename CpuTy, typename PETy> class PtrencdecIsa {
    using SelfTy = PtrencdecIsa<CpuTy, PETy>;

 public:
    static constexpr uint8_t kInstByte0 = 0xF0;  // LOCK prefix
    static constexpr uint8_t kInstByte2 = 0x01;
    static constexpr uint8_t kInstByte3Min = 0xc0;

    bool m_debug_ = false;
    bool m_enable_encoding_ = true;
    bool m_enable_decoding_ = false;

 protected:
    CpuTy *const m_cpu_;
    PETy *const m_pe_;

 public:
    inline PtrencdecIsa(CpuTy *cpu, PETy *pe) : m_cpu_(cpu), m_pe_(pe) {
        xed_tables_init();
    }

    virtual ~PtrencdecIsa() {}

    /**
     * @brief Call once to register illegal_instruction_cb for Simics
     *
     * @param con
     */
    inline void register_callbacks(conf_object_t *con) {
        m_cpu_->register_illegal_instruction_cb(
                con,
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
        cpu_bytes_t bytes = m_cpu_->get_instruction_bytes(iq_handle);

        if (bytes.size == 0) {
            return -1;
        }
        if (bytes.data[0] != kInstByte0) {
            return 0;
        }

        if (bytes.size < 2) {
            return -2;
        }
        if (bytes.size < 3) {
            return -3;
        }
        if (bytes.data[2] != kInstByte2) {
            return 0;
        }

        if (bytes.size < 4) {
            return -4;
        }

        if (bytes.data[3] >= kInstByte3Min) {
            emul_data_t *cbdata = MM_ZALLOC(1, emul_data_t);

            // Store the register op numbers in cbdata
            auto res = xed_decode_register(bytes.data, 4, &cbdata->regs_);
            ASSERT_FMT(res >= 0, "failed to get regs from %02x%02x%02x%02x",
                       bytes.data[0], bytes.data[1], bytes.data[2],
                       bytes.data[3]);
            // Store pointer to self in cbdata
            cbdata->self_ = static_cast<void *>(this);

            const int ptr_reg = cbdata->regs_.pointer_reg_;
            const int size_reg = cbdata->regs_.size_reg_;

            // Trigger encryption when ptr and size are different reg oprs
            // otherwise trigger decryption (this is obv. temp. hack)
            if (ptr_reg != size_reg) {
                ifdbgprint(m_debug_,
                           "detected CCPTRENC at %lx "
                           "with opcode: %02x %02x %02x %02x (regs: %d, %d)\n",
                           m_cpu_->read_rip(), bytes.data[0], bytes.data[1],
                           bytes.data[2], bytes.data[3], ptr_reg, size_reg);

                m_cpu_->register_emulation_cb(
                        [](auto *obj, auto *cpu, void *cbdata) {
                            auto *emul_data =
                                    static_cast<emul_data_t *>(cbdata);
                            auto *self =
                                    static_cast<SelfTy *>(emul_data->self_);
                            return self->ccptrenc_emulation(&emul_data->regs_);
                        },
                        handle, cbdata,
                        [](auto *obj, auto *cpu, auto *data) {
                            MM_FREE(data);
                        });
            } else {
                ifdbgprint(m_debug_,
                           "detected CCPTRDEC at %lx "
                           "with opcode: %02x %02x %02x %02x (regs: %d, %d)\n",
                           m_cpu_->read_rip(), bytes.data[0], bytes.data[1],
                           bytes.data[2], bytes.data[3], ptr_reg, size_reg);

                m_cpu_->register_emulation_cb(
                        [](auto *obj, auto *cpu, void *cbdata) {
                            auto *emul_data =
                                    static_cast<emul_data_t *>(cbdata);
                            auto *self =
                                    static_cast<SelfTy *>(emul_data->self_);
                            return self->ccptrdec_emulation(&emul_data->regs_);
                        },
                        handle, cbdata,
                        [](auto *obj, auto *cpu, auto *data) {
                            MM_FREE(data);
                        });
            }
            return 4;
        }
        return 0;
    }

    inline cpu_emulation_t ccptrenc_emulation(void *cbdata) {
        const auto *regs = static_cast<cc_ptrenc_regs_t *>(cbdata);
        const int ptr_reg = regs->pointer_reg_;
        const int size_reg = regs->size_reg_;

        uint64_t ptr = m_cpu_->get_gpr(ptr_reg);
        ptr_metadata_t ptr_metadata = {.uint64_ = m_cpu_->get_gpr(size_reg)};

        uint64_t ptr_encoded;

        if (!m_enable_encoding_) {
            ptr_encoded = ptr;
        } else {
            ptr_encoded = m_pe_->encode_pointer(ptr, &ptr_metadata);
        }

        m_cpu_->set_gpr(ptr_reg, ptr_encoded);

        ifdbgprint(m_debug_,
                   "CCPTRENC at 0x%016lx operands %d, %d:\n"
                   "       size:         %lu\n"
                   "       version:      %lu\n"
                   "       input ptr  <- 0x%016lx\n"
                   "       output ptr -> 0x%016lx",
                   m_cpu_->read_rip(), ptr_reg, size_reg, ptr_metadata.size_,
                   ptr_metadata.version_, ptr, ptr_encoded);
        return CPU_Emulation_Fall_Through;
    }

    inline cpu_emulation_t ccptrdec_emulation(void *cbdata) {
        const auto *regs = static_cast<cc_ptrenc_regs_t *>(cbdata);
        const int ptr_reg = regs->pointer_reg_;
        const int size_reg = regs->size_reg_;

        const uint64_t ptr_encoded = m_cpu_->get_gpr(ptr_reg);
        const uint64_t ptr_decoded = m_pe_->decode_pointer(ptr_encoded);

        m_cpu_->set_gpr(ptr_reg, ptr_decoded);

        ifdbgprint(m_debug_,
                   "CCPTRDEC at 0x%16lx operands %d, %d:\n"
                   "       input ptr  <- 0x%016lx\n"
                   "       output ptr -> 0x%016lx",
                   m_cpu_->read_rip(), ptr_reg, size_reg, ptr_encoded,
                   ptr_decoded);
        return CPU_Emulation_Fall_Through;
    }

    static inline tuple_int_string_t disassemble(generic_address_t addr,
                                                 cpu_bytes_t bytes) {
        if (bytes.size == 0) {
            return {-1, NULL};
        }
        if (bytes.data[0] != kInstByte0) {
            return {0, NULL};
        }

        if (bytes.size < 2) {
            return {-2, NULL};
        }
        if (bytes.size < 3) {
            return {-3, NULL};
        }
        if (bytes.data[2] != kInstByte2) {
            return {0, NULL};
        }

        if (bytes.size < 4) {
            return {-4, NULL};
        }

        if (bytes.data[3] >= kInstByte3Min) {
            cc_ptrenc_regs_t *cc_isa_regs = MM_ZALLOC(1, cc_ptrenc_regs_t);
            xed_decode_register(bytes.data, 4, cc_isa_regs);

            if (cc_isa_regs->pointer_reg_ != cc_isa_regs->size_reg_) {
                return {4, MM_STRDUP("ccptrenc")};
            }
            return {4, MM_STRDUP("ccptrdec")};
        }
        return {0, NULL};
    }

    static inline int convert_xed_reg_to_simics(xed_reg_enum_t xed_reg) {
        switch (xed_reg) {
        case XED_REG_RAX:
            return 0;
        case XED_REG_RCX:
            return 1;
        case XED_REG_RDX:
            return 2;
        case XED_REG_RBX:
            return 3;
        case XED_REG_RSP:
            return 4;
        case XED_REG_RBP:
            return 5;
        case XED_REG_RSI:
            return 6;
        case XED_REG_RDI:
            return 7;
        case XED_REG_R8:
            return 8;
        case XED_REG_R9:
            return 9;
        case XED_REG_R10:
            return 10;
        case XED_REG_R11:
            return 11;
        case XED_REG_R12:
            return 12;
        case XED_REG_R13:
            return 13;
        case XED_REG_R14:
            return 14;
        case XED_REG_R15:
            return 15;
        default:
            return -1;
        }
    }

    static inline int xed_decode_register(const uint8_t *opcode, int size,
                                          cc_ptrenc_regs_t *cc_ptrenc_regs) {
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
        cc_ptrenc_regs->pointer_reg_ = convert_xed_reg_to_simics(reg0);
        cc_ptrenc_regs->size_reg_ = convert_xed_reg_to_simics(reg1);
        if (cc_ptrenc_regs->pointer_reg_ == -1 ||
            cc_ptrenc_regs->size_reg_ == -1) {
            ASSERT_MSG(false, "failed to convert XED registers");
            return -1;
        }
        return 0;
    }
};

}  // namespace ccsimics

#endif  // MODULES_COMMON_CCSIMICS_PTRENCDEC_ISA_H_
