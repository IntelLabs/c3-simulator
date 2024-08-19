// Copyright 2016-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef MODULES_COMMON_CCSIMICS_X86_CONTEXT_H_
#define MODULES_COMMON_CCSIMICS_X86_CONTEXT_H_

#include "ccsimics/context.h"
#include "ccsimics/simics_connection.h"
#include "ccsimics/x86_simics_connection.h"
#include "c3/crypto/cc_encoding.h"

class X86Context : public Context {
 public:
    X86Context(SimicsConnection *con, CCPointerEncodingBase *ptrenc)
        : Context(con, ptrenc) {}
    virtual ~X86Context() = default;

    /**
     * @brief Registers callbacks for the ISA extensions
     *
     * @tparam CtxTy Type used for void * casts
     */
    template <typename CtxTy> inline void enable() {
        con_->register_illegal_instruction_cb(
                /* decode */
                [](conf_object_t *obj, conf_object_t *cpu,
                   decoder_handle_t *handle, instruction_handle_t *iq_handle,
                   void *ctx) {
                    auto c = static_cast<CtxTy *>(ctx);
                    return c->template decode<CtxTy>(handle, iq_handle);
                },
                /* disassemble */
                [](conf_object_t *con, conf_object_t *cpu,
                   generic_address_t addr, cpu_bytes_t bytes) {
                    return CtxTy::disassemble(addr, bytes);
                },
                static_cast<void *>(this));
    }

    /**
     * @brief Callback for instruction decoder
     *
     * @tparam CtxTy
     * @param handle
     * @param iq_handle
     * @return int
     */
    template <typename CtxTy>
    inline int decode(decoder_handle_t *handle,
                      instruction_handle_t *iq_handle) {
        auto bytes = con_->get_instruction_bytes(iq_handle);

        if (bytes.size == 0) {
            return -1;
        }
        if (bytes.data[0] != kInstByte0) {
            return 0;
        }
        if (bytes.size < 2) {
            return -2;
        }

        if (bytes.data[1] == kInstByte1Save) {
            con_->register_emulation_cb(
                    [](conf_object_t *obj, conf_object_t *cpu, void *ud) {
                        return static_cast<CtxTy *>(ud)->save_context();
                    },
                    handle, static_cast<void *>(this), NULL);
            return 2;
        }
        if (bytes.data[1] == kInstByte1Load) {
            con_->register_emulation_cb(
                    [](conf_object_t *obj, conf_object_t *cpu, void *ud) {
                        return static_cast<CtxTy *>(ud)->load_context();
                    },
                    handle, static_cast<void *>(this), NULL);
            return 2;
        }
        return 0;
    }

    /**
     * @brief Disassemble callback
     *
     * This will be invoked e.g., when using the Simics console `disassemble`
     * command to disassemble code running within the guest.
     *
     * @param addr
     * @param bytes
     * @return tuple_int_string_t
     */
    inline static tuple_int_string_t disassemble(generic_address_t addr,
                                                 cpu_bytes_t bytes) {
        if (bytes.size == 0) {
            return {-1, NULL};
        }
        if (bytes.data[0] != kInstByte0) {
            return {0, NULL};
        }
        if (bytes.size == 1) {
            return {-2, NULL};
        }

        if (bytes.data[1] == kInstByte1Save) {
            return {2, MM_STRDUP("cc_ctx_save")};
        }
        if (bytes.data[1] == kInstByte1Load) {
            return {2, MM_STRDUP("cc_ctx_load")};
        }

        return {0, NULL};
    }

    /**
     * @brief Implements the save_context ISA
     *
     * NOTE: This needs to be public since it is called form the callback.
     *
     * @return cpu_emulation_t
     */
    inline virtual cpu_emulation_t save_context() {
        uint8_t local_buff[sizeof(struct cc_context)];
        const uint64_t rax =
                reinterpret_cast<X86SimicsConnection *>(con_)->read_rax();
        const uint64_t addr = m_ptrenc_->decode_pointer_if_encoded(rax);

        ifdbgprint(kTrace, "Writing C3 context into 0x%016lx (at IP: 0x%016lx)",
                   addr,
                   reinterpret_cast<X86SimicsConnection *>(con_)->read_rip());

        // Save/load via temporary buffer if I/O buffer is encrypted
        uint8_t *buff = is_encoded_cc_ptr(rax)
                                ? reinterpret_cast<uint8_t *>(local_buff)
                                : reinterpret_cast<uint8_t *>(&cc_context_);

        if (is_encoded_cc_ptr(rax)) {
            // Encrypt internal cc_context into temp buffer
            CCDataEncryption::encrypt_decrypt_many_bytes(
                    addr, get_data_key(addr),
                    reinterpret_cast<uint8_t *>(&cc_context_), buff,
                    sizeof(struct cc_context));
        }

        con_->write_mem(addr, addr + sizeof(struct cc_context),
                        reinterpret_cast<char *>(&cc_context_));

        return CPU_Emulation_Fall_Through;
    }

    /**
     * @brief Implements the load_context ISA
     *
     * NOTE: This needs to be public since it is called form the callback.
     *
     * @return cpu_emulation_t
     */
    inline virtual cpu_emulation_t load_context() {
        uint8_t local_buff[sizeof(struct cc_context)];
        const uint64_t rax =
                reinterpret_cast<X86SimicsConnection *>(con_)->read_rax();
        const uint64_t addr = m_ptrenc_->decode_pointer_if_encoded(rax);

        ifdbgprint(kTrace, "Loading C3 context from 0x%016lx (at IP: 0x%016lx)",
                   addr,
                   reinterpret_cast<X86SimicsConnection *>(con_)->read_rip());

        // Save/load via temporary buffer if I/O buffer is encrypted
        uint8_t *buff = is_encoded_cc_ptr(rax)
                                ? reinterpret_cast<uint8_t *>(local_buff)
                                : reinterpret_cast<uint8_t *>(&cc_context_);

        con_->read_mem(addr, addr + sizeof(struct cc_context),
                       reinterpret_cast<char *>(buff));

        if (is_encoded_cc_ptr(rax)) {
            // Decrypt temp buffer into internal cc_context
            CCDataEncryption::encrypt_decrypt_many_bytes(
                    addr, get_data_key(addr),
                    reinterpret_cast<uint8_t *>(&cc_context_), buff,
                    sizeof(struct cc_context));
        }

        set_data_key(&dp_key_, cc_context_.dp_key_bytes_);
        set_data_key(&c_key_, cc_context_.c_key_bytes_);

        if (!kFixedSharedKey) {
            set_data_key(&ds_key_, cc_context_.ds_key_bytes_);
        }
        if (!kFixedAddrKey) {
            set_addr_key(&addr_key_, cc_context_.addr_key_bytes_);
        }

        con_->ctx_loaded_cb();
        return CPU_Emulation_Fall_Through;
    }
};

#endif  // MODULES_COMMON_CCSIMICS_X86_CONTEXT_H_
