/*
 Copyright 2023 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_COMMON_CCSIMICS_CASTACK_H_
#define MODULES_COMMON_CCSIMICS_CASTACK_H_

#include <typeinfo>
#include "ccsimics/simics_util.h"
#include "ccsimics/stack_hardening.h"

namespace ccsimics {

template <typename ConTy, typename PtrEncTy>
class CaStack final : public StackHardening {
    using SelfTy = CaStack<ConTy, PtrEncTy>;

    enum Inst { FXSAVE, FXRSTOR, JMPRDX, OTHER };

 public:
    CaStack(ConTy *con, PtrEncTy *ptrenc) : con_(con), ptrenc_(ptrenc) {
        SIM_printf("[C3] Loaded CAStack\n");
        ifdbgprint(con_->debug_on, "ConTy: %s, PtrEncTy: %s",
                   typeid(ConTy).name(), typeid(PtrEncTy).name());
    }

    inline bool check_memory_access(logical_address_t, uint64_t,
                                    enum RW) final {
        return true;
    }

    inline uint64_t get_data_tweak(logical_address_t la) final { return la; }

    inline bool is_encoded_sp(const uint64_t ptr) final {
        /* Stack pointers same as regular CAs. */
        return false;
    }

    inline uint64_t decode_sp(const uint64_t ptr) final {
        return ptrenc_->decode_pointer(ptr);
    }

    inline void instruction_before_cb(instruction_handle_t *instr_handle) {
        this->current_instruction_ = decode_opcode(instr_handle);

        switch (this->current_instruction_) {
        case Inst::FXSAVE:
        case Inst::FXRSTOR: {
            ifdbgprint(con_->debug_on, "Fixing FXSAVE/FXRSTOR");
            con_->write_rsp(save_and_decode_if_encoded(con_->read_rsp()));
            break;
        }
        case Inst::JMPRDX: {
            ifdbgprint(con_->debug_on, "Fixing JMPRDX");
            con_->write_rdx(save_and_decode_if_encoded(con_->read_rdx()));
            break;
        }
        default: {
        }
        }
    }

    inline void instruction_after_cb(instruction_handle_t *instr_handle) {
        switch (this->current_instruction_) {
        case Inst::FXSAVE:
        case Inst::FXRSTOR: {
            con_->write_rsp(saved_reg_);
            break;
        }
        case Inst::JMPRDX: {
            con_->write_rdx(saved_reg_);
            break;
        }
        default: {
        }
        }
    }

    inline void enable_callbacks() final {
        if (inst_before_cb_ != nullptr) {
            con_->ci_iface->enable_callback(con_->cpu_, inst_before_cb_);
            con_->ci_iface->enable_callback(con_->cpu_, inst_after_cb_);
        } else {
            inst_before_cb_ = con_->register_instruction_before_cb(
                    [](auto *, auto *, auto *i, void *d) {
                        static_cast<SelfTy *>(d)->instruction_before_cb(i);
                    },
                    static_cast<void *>(this));
            inst_after_cb_ = con_->register_instruction_after_cb(
                    [](auto *, auto *, auto *i, void *d) {
                        static_cast<SelfTy *>(d)->instruction_after_cb(i);
                    },
                    static_cast<void *>(this));
        }
    }

    inline void disable_callbacks() final {
        if (inst_before_cb_ != nullptr) {
            con_->ci_iface->disable_callback(con_->cpu_, inst_before_cb_);
            con_->ci_iface->disable_callback(con_->cpu_, inst_after_cb_);
        }
    }

 private:
    ConTy *con_;
    PtrEncTy *ptrenc_;

    enum Inst current_instruction_ = OTHER;
    uint64_t saved_reg_ = 0x0;

    cpu_cb_handle_t *inst_before_cb_ = nullptr;
    cpu_cb_handle_t *inst_after_cb_ = nullptr;

    auto decode_opcode(instruction_handle_t *instr) {
        static constexpr char Fxsave2Bytes[2] = {'\x0F', '\xAE'};
        static constexpr char Jmprdx2Bytes[2] = {'\xFF', '\xE2'};

        const cpu_bytes_t opcode = con_->get_instruction_bytes(instr);
        if (opcode.size >= 3) {
            const auto *bytes = opcode.data;

            if (compare_bytes(reinterpret_cast<const uint8_t *>(Fxsave2Bytes),
                              bytes, 2)) {
                modrm_t modrm = {.u8_ = bytes[2]};
                if (modrm.reg_ == 0) {
                    return Inst::FXSAVE;
                }
                if (modrm.reg_ == 1) {
                    return Inst::FXRSTOR;
                }
            }

            if (compare_bytes(reinterpret_cast<const uint8_t *>(Jmprdx2Bytes),
                              bytes, 2)) {
                return Inst::JMPRDX;
            }
        }

        return Inst::OTHER;
    }

    inline uint64_t save_and_decode_if_encoded(uint64_t reg_value) {
        saved_reg_ = reg_value;
        return is_encoded_cc_ptr(reg_value) ? ptrenc_->decode_pointer(reg_value)
                                            : reg_value;
    }
};

}  // namespace ccsimics

#endif  // MODULES_COMMON_CCSIMICS_CASTACK_H_
