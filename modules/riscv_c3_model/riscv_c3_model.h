// Copyright 2024-2025 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef MODULES_RISCV_C3_MODEL_RISCV_C3_MODEL_H_
#define MODULES_RISCV_C3_MODEL_RISCV_C3_MODEL_H_

#include <cassert>
#include <memory>
#include <string>
#include <simics/simulator/control.h>
#include <simics/simulator/memory.h>
extern "C" {
#include <xed-interface.h>
}
#include "ccsimics/c3_base_model.h"
#include "ccsimics/data_encryption.h"
#include "ccsimics/simics_util.h"

namespace ccsimics {

template <typename ConTy, typename CtxTy, typename PtrEncTy>
class RiscvC3Model : public C3BaseModel<ConTy, CtxTy, PtrEncTy> {
    using BaseTy = C3BaseModel<ConTy, CtxTy, PtrEncTy>;

    bool need_fixup = false;
    bool exception_fixup_only = false;
    uint32_t fixup_reg_num = 0;
    uint64_t fixup_reg_val = 0;

 public:
    RiscvC3Model(ConTy *con, CtxTy *ctx, PtrEncTy *ptrenc)
        : C3BaseModel<ConTy, CtxTy, PtrEncTy>(con, ctx, ptrenc) {}

    template <typename HandlerTy> inline void register_callbacks(ConTy *con) {
        BaseTy::template register_read_write_cb<HandlerTy>(con);

        this->con_->register_instruction_before_cb(
                [](auto *, auto *, auto *i, void *d) {
                    static_cast<decltype(this)>(d)->instruction_before_cb(i);
                },
                static_cast<void *>(this));
        this->con_->register_instruction_after_cb(
                [](auto *, auto *, auto *i, void *d) {
                    static_cast<decltype(this)>(d)->instruction_after_cb(i);
                },
                static_cast<void *>(this));
        this->con_->register_exception_before_cb(
                CPU_Exception_All,
                [](auto *, auto *, auto *h, auto *d) {
                    static_cast<decltype(this)>(d)->exception_before_cb(h);
                },
                static_cast<void *>(this));
    }

    inline uint64_t encrypt_decrypt_u64(const uint64_t tweak,
                                        const uint64_t val,
                                        const data_key_t *k) const {
        uint8 buff[8];
        uint8 input_buff[8];
        ptr_metadata_t md;

        {
            uint64_t tmp_val = val;
            for (int i = 0; i < 8; ++i) {
                input_buff[i] = tmp_val & 0xff;
                tmp_val = tmp_val >> 8;
            }
        }

        cpu_bytes_t input{.size = 8, .data = input_buff};

        input.data = input_buff;

        cpu_bytes_t bytes_mod =
                encrypt_decrypt_bytes(&md, tweak, k, input, buff);
        uint64_t result = 0;
        for (int i = 0; i < 8; ++i) {
            result = result | (((uint64_t)bytes_mod.data[i]) << (i * 8));
        }
        return result;
    }

    inline uint64_t encrypt_decrypt_u64(uint64_t tweak, uint64_t val) const {
        return encrypt_decrypt_u64(tweak, val, this->ctx_->get_dp_key());
    }

    inline logical_address_t decode_pointer(logical_address_t address) final {
        if (!this->ctx_->cc_enabled()) {
            return address;
        }

        return C3BaseModel<ConTy, CtxTy, PtrEncTy>::decode_pointer(address);
    }

    void instruction_before_cb(instruction_handle_t *h) {
        if (!this->ctx_->cc_enabled()) {
            return;
        }

        cpu_bytes_t opcode = this->con_->get_instruction_bytes(h);
        const uint8_t *opcode_bytes = opcode.data;

        // Decode RISC-V instruction
        uint32_t instruction = 0;
        for (size_t i = 0; i < opcode.size; ++i) {
            instruction |= (opcode_bytes[i] << (i * 8));
        }

        // Extract opcode field (bits 0-6)
        uint32_t opcode_field = instruction & 0x7F;

        // RISC-V Load and Store opcodes
        const uint32_t LOAD_OPCODE = 0x03;
        const uint32_t STORE_OPCODE = 0x23;

        if (opcode_field != LOAD_OPCODE && opcode_field != STORE_OPCODE) {
            return;
        }

        // Extract register number of the pointer operand (bits 15-19 for rs1)
        uint32_t reg_num = (instruction >> 15) & 0x1F;

        // Extract register number of the destination operand (bits 7-11 for rd)
        uint32_t dst_reg_num = (instruction >> 7) & 0x1F;

        // Read the address and use address_before do decode
        uint64_t addr = this->con_->read_reg(reg_num);
        uint64_t addr_enc = this->address_before(addr, nullptr);

        // Modify the reg val if it was a CA (i.e., address_before changed it)
        if (addr != addr_enc) {
            if (this->con_->debug_on) {
                const char *reg_name = this->con_->ir_iface->get_name(
                        this->con_->cpu_, reg_num);
                SIM_printf("Decoding register %s to 0x%016lx (old value: "
                           "0x%016lx)\n",
                           reg_name, addr_enc, addr);
            }

            this->con_->write_reg(reg_num, addr_enc);

            // Store values to restore after instruction in restore_fixup()
            need_fixup = true;
            fixup_reg_num = reg_num;
            fixup_reg_val = addr;
            // If out and dst reg are the same, we only fixup on exception
            exception_fixup_only = (dst_reg_num == reg_num);
        }
    }

    inline void exception_before_cb(exception_handle_t *) {
        restore_fixup(true);
    }

    inline void instruction_after_cb(instruction_handle_t *) {
        restore_fixup(false);
    }

 protected:
    inline void restore_fixup(bool is_exception) {
        if (!need_fixup || !this->ctx_->cc_enabled()) {
            return;
        }

        if (!is_exception && exception_fixup_only) {
            return;
        }

        if (this->con_->debug_on) {
            const char *reg_name = this->con_->ir_iface->get_name(
                    this->con_->cpu_, fixup_reg_num);
            SIM_printf("Restoring register %s to 0x%016lx\n", reg_name,
                       fixup_reg_val);
        }

        this->con_->write_reg(fixup_reg_num, fixup_reg_val);
        need_fixup = false;
    }

 public:
    static inline void register_attributes(conf_class_t *cl) {}
};

}  // namespace ccsimics

#endif  // MODULES_RISCV_C3_MODEL_RISCV_C3_MODEL_H_
