/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/

#ifndef MODULES_COMMON_CCSIMICS_CONTEXT_H_
#define MODULES_COMMON_CCSIMICS_CONTEXT_H_

#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <simics/arch/x86-instrumentation.h>
#include <simics/base/log.h>
#include <simics/model-iface/cpu-instrumentation.h>
#include <simics/model-iface/int-register.h>
#include <simics/model-iface/processor-info.h>
#include <simics/simulator-api.h>
#include <simics/simulator/conf-object.h>
#include <simics/simulator/output.h>
#include "ccsimics/data_encryption.h"
#include "ccsimics/simics_connection.h"
#include "ccsimics/simics_util.h"
#include "crypto/ascon_cipher.h"
#include "crypto/cc_encoding.h"
#include "malloc/cc_globals.h"

#define DEF_DATA_KEY_BYTES                                                     \
    {                                                                          \
        0xb5, 0x82, 0x4d, 0x03, 0x17, 0x5c, 0x25, 0x2a, 0xfc, 0x71, 0x1e,      \
                0x01, 0x02, 0x60, 0x87, 0x91                                   \
    }

#define DEF_ADDR_KEY_BYTES                                                     \
    {                                                                          \
        0xd1, 0xbe, 0x2c, 0xdb, 0xb5, 0x82, 0x4d, 0x03, 0x17, 0x5c, 0x25,      \
                0x2a, 0x20, 0xb6, 0xf2, 0x93, 0xfd, 0x01, 0x96, 0xe7, 0xb5,    \
                0xe6, 0x88, 0x1c, 0xb3, 0x69, 0x22, 0x60, 0x38, 0x09, 0xf6,    \
                0x68                                                           \
    }

/**
 * @brief Implements ISA for managing C3 configuration.

 * The Context object internally maintains track of CPU-specific C3
 * configuration such as keys and whether to enable the functionality. It also
 * implements callbacks for ISA extensions that provide instructions for saving
 * and restoring the C3 configuration to memory within the guest (e.g., during
 * context switches, or when initializing keys on process startup).
 *
 */
class Context {
 public:
    static constexpr size_t kDataKeyBytesSize =
            sizeof_field(data_key_t, bytes_);
    static constexpr size_t kAddrKeyBytesSize =
            sizeof_field(pointer_key_t, bytes_);
    static constexpr size_t kMaxKeyByteSize =
            (kDataKeyBytesSize > kAddrKeyBytesSize ? kDataKeyBytesSize
                                                   : kAddrKeyBytesSize);
    static constexpr uint64_t kDataTweak = 0x1234abcd5678efab;
    static constexpr access_t kRwAccess =
            (access_t)(Sim_Access_Read | Sim_Access_Write);

    typedef struct cc_context context_t;

    static constexpr const bool kFixedSharedKey = false;
    static constexpr const bool kFixedAddrKey = false;

    static constexpr uint8_t kInstByte0 = 0xF0;      // LOCK prefix
    static constexpr uint8_t kInstByte1Save = 0x2F;  // DAS
    static constexpr uint8_t kInstByte1Load = 0xFA;  // CLI

 protected:
    // Set keys to static default values, the constructor then inovkes the
    // key schedule initalization, and kernel (if enabled) sets new keys.
    data_key_t ds_key_{.size_ = kDataKeyBytesSize,
                       .bytes_ = DEF_DATA_KEY_BYTES};
    data_key_t dp_key_{.size_ = kDataKeyBytesSize,
                       .bytes_ = DEF_DATA_KEY_BYTES};
    data_key_t c_key_{.size_ = kDataKeyBytesSize, .bytes_ = DEF_DATA_KEY_BYTES};
    pointer_key_t addr_key_{.size_ = kAddrKeyBytesSize,
                            .bytes_ = DEF_ADDR_KEY_BYTES};

    // The context struct corresponding to the OS view of the ctx, i.e., in
    // contrst to ds_key, dp_key, etc. cc_context does not contain key
    // schedules or such internal information.
    context_t cc_context_{.ds_key_bytes_ = DEF_DATA_KEY_BYTES,
                          .dp_key_bytes_ = DEF_DATA_KEY_BYTES,
                          .c_key_bytes_ = DEF_DATA_KEY_BYTES,
                          .addr_key_bytes_ = DEF_ADDR_KEY_BYTES};

    SimicsConnection *con_;
    CCPointerEncoding *const m_ptrenc_;

 public:
    inline explicit Context(SimicsConnection *con, CCPointerEncoding *ptrenc)
        : con_(con), m_ptrenc_(ptrenc) {
        cc_context_.flags_.raw_ = 0;
        init_addr_key(&addr_key_);

        // Enable CC by default since this has been a long-standing assumption.
        // For heap, the allocator separately controls whether to actually
        // procude CAs or LAs for heap allocations.
        set_cc_enabled(true);
    }

    virtual inline ~Context() = default;

    inline context_t *get_raw_ctx() { return &cc_context_; }

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
     * @brief Get the shared data key
     *
     * @return const data_key_t*
     */
    inline const data_key_t *get_ds_key() const { return &ds_key_; }
    /**
     * @brief Get the private data key
     *
     * @return const data_key_t*
     */
    inline const data_key_t *get_dp_key() const { return &dp_key_; }
    /**
     * @brief Get the code key
     *
     * @return const data_key_t*
     */
    inline const data_key_t *get_c_key() const { return &c_key_; }
    /**
     * @brief Get the address/pointer key
     *
     * @return const pointer_key_t*
     */
    inline const pointer_key_t *get_addr_key() { return &addr_key_; }

    /**
     * @brief Get a Simics attr_data object representing the Context state
     *
     * @return attr_value_t
     */
    inline attr_value_t get_attr_data() {
        std::memcpy(cc_context_.dp_key_bytes_, dp_key_.bytes_,
                    kDataKeyBytesSize);
        std::memcpy(cc_context_.ds_key_bytes_, ds_key_.bytes_,
                    kDataKeyBytesSize);
        std::memcpy(cc_context_.c_key_bytes_, c_key_.bytes_, kDataKeyBytesSize);
        std::memcpy(cc_context_.addr_key_bytes_, addr_key_.bytes_,
                    kAddrKeyBytesSize);
        return SIM_make_attr_data(sizeof(context_t), &cc_context_);
    }

    /**
     * @brief Set the Context state from Simics attr_data object
     *
     * @param attr
     * @return set_error_t
     */
    inline set_error_t set_from_attr_data(attr_value_t *attr) {
        ASSERT_FMT(SIM_attr_data_size(*attr) != sizeof(context_t),
                   "Bad data size in attr_value_t, expected %lu got %u",
                   sizeof(context_t), SIM_attr_data_size(*attr));
        const auto *data =
                reinterpret_cast<const context_t *>(SIM_attr_data(*attr));
        cc_context_.flags_.raw_ = data->flags_.raw_;
        cc_context_.reserved1_ = data->reserved1_;
        set_data_key(&ds_key_, data->ds_key_bytes_);
        set_data_key(&dp_key_, data->dp_key_bytes_);
        set_data_key(&c_key_, data->c_key_bytes_);
        set_addr_key(&addr_key_, data->addr_key_bytes_);
        return Sim_Set_Ok;
    }

    /**
     * @brief Implements the save_context ISA
     *
     * NOTE: This needs to be public since it is called form the callback.
     *
     * @return cpu_emulation_t
     */
    inline virtual cpu_emulation_t save_context() {
        const uint64_t rax_val = con_->read_rax();
        con_->write_mem(rax_val, rax_val + sizeof(struct cc_context),
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
        const uint64_t rax_val = con_->read_rax();
        con_->read_mem(rax_val, rax_val + sizeof(struct cc_context),
                       reinterpret_cast<char *>(&cc_context_));

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
     * @brief Get cc_context.cc_enabled
     *
     * NOTE: This is currently ignored by all models!
     *
     * @return true
     * @return false
     */
    inline bool cc_enabled() const {
        return cc_context_.flags_.bitfield_.cc_enabled_;
    }

    /**
     * @brief Set cc_context.cc_enabled
     *
     * @param val
     */
    inline void set_cc_enabled(bool val) {
        cc_context_.flags_.bitfield_.cc_enabled_ = val;
    }

#ifdef CC_INTEGRITY_ENABLE
    /**
     * @brief Set cc_context.icv_lock
     *
     * @param val
     */
    inline void set_icv_enabled(bool val) {
        cc_ctx_set_icv_enabled(&cc_context_, val);
    }

    /**
     * @brief Set cc_context.icv_lock
     *
     * @param val
     */
    inline bool get_icv_enabled() {
        return cc_ctx_get_icv_enabled(&cc_context_);
    }

    inline bool get_and_zero_icv_map_reset() {
        const bool r = cc_ctx_get_icv_map_reset(&cc_context_);
        cc_ctx_set_icv_map_reset(&cc_context_, 0);
        return r;
    }

    /**
     * @brief Set cc_context.icv_lock
     *
     * @param val
     */
    inline void set_icv_lock(bool val) {
        cc_ctx_set_icv_lock(&cc_context_, val);
    }

    /**
     * @brief Get cc_context.icv_lock
     *
     * @return true
     * @return false
     */
    inline bool get_icv_lock() const {
        return cc_ctx_get_icv_lock(&cc_context_);
    }
#endif  // CC_INTEGRITY_ENABLE

#ifdef CC_SHADOW_RIP_ENABLE
    inline void set_gsrip_enabled(bool val) {
        cc_ctx_set_shadow_rip_enabled(&cc_context_, val);
    }

    inline bool get_gsrip_enabled() const {
        return cc_ctx_get_shadow_rip_enabled(&cc_context_);
    }

    inline uint64_t get_gsrip() const {
        return cc_ctx_get_shadow_rip<uint64_t>(&cc_context_);
    }

    inline void set_gsrip(uint64_t val) {
        return cc_ctx_set_shadow_rip<uint64_t>(&cc_context_, val);
    }
#endif  // CC_SHADOW_RIP_ENABLE

    /**
     * @brief Debug function that dumps keys to Simics console
     *
     */
    inline void dump_keys() const {
        SIM_printf(
                "%-20s: %s\n", "addr_key.bytes_\n",
                buf_to_hex_string(addr_key_.bytes_, kAddrKeyBytesSize).c_str());
        SIM_printf(
                "%-20s: %s\n", "ds_key_.bytes_\n",
                buf_to_hex_string(ds_key_.bytes_, kDataKeyBytesSize).c_str());
        SIM_printf(
                "%-20s: %s\n", "dp_key_.bytes_\n",
                buf_to_hex_string(dp_key_.bytes_, kDataKeyBytesSize).c_str());
        SIM_printf("%-20s: %s\n", "c_key_.bytes_\n",
                   buf_to_hex_string(c_key_.bytes_, kDataKeyBytesSize).c_str());
    }

 private:
    /**
     * @brief Initialized address key
     *
     * @param key
     */
    inline void init_addr_key(pointer_key_t *key) {
        m_ptrenc_->init_pointer_key(key->bytes_, key->size_);
    }

    /**
     * @brief Set the address key, and initialize it
     *
     * @param key
     * @param bytes
     * @return true
     * @return false
     */
    inline bool set_addr_key(pointer_key_t *key, const uint8_t *bytes) {
        if (std::memcmp(&key->bytes_, bytes, kAddrKeyBytesSize) == 0) {
            return false;
        }
        memcpy(static_cast<void *>(&key->bytes_),
               static_cast<const void *>(bytes), kAddrKeyBytesSize);
        init_addr_key(key);
        return true;
    }

 public:
    template <typename ConTy>
    static inline void register_attributes(conf_class_t *cl) {
        SIM_register_typed_attribute(
                cl, "c3_context",
                [](auto *, auto *obj, auto *) -> attr_value_t {
                    auto *con = reinterpret_cast<ConTy *>(SIM_object_data(obj));
                    return con->ctx_->get_attr_data();
                },
                nullptr,
                [](auto *, auto *obj, auto *val, auto *) -> set_error_t {
                    auto *con = reinterpret_cast<ConTy *>(SIM_object_data(obj));
                    return con->ctx_->set_from_attr_data(val);
                },
                nullptr, Sim_Attr_Optional, "d", NULL, "c3_context data");
    }

 private:
    /**
     * @brief Set a data key to given bytes
     *
     * @param key
     * @param bytes
     * @return true
     * @return false
     */
    static inline bool set_data_key(data_key_t *key, const uint8_t *bytes) {
        if (std::memcmp(&key->bytes_, bytes, kDataKeyBytesSize) == 0) {
            return false;
        }
        memcpy(static_cast<void *>(&key->bytes_),
               static_cast<const void *>(bytes), kDataKeyBytesSize);
        return true;
    }

    // Some sanity checks to detect size mistmatches early
    static_assert(kDataKeyBytesSize == sizeof(data_key_bytes_t),
                  "bad constants");
    static_assert(kAddrKeyBytesSize == sizeof(pointer_key_bytes_t),
                  "bad constants");
};

#endif  // MODULES_COMMON_CCSIMICS_CONTEXT_H_
