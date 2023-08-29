/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_COMMON_CCSIMICS_C3_BASE_MODEL_H_
#define MODULES_COMMON_CCSIMICS_C3_BASE_MODEL_H_

#include <simics/model-iface/cpu-instrumentation.h>
#include <simics/processor/types.h>
#include <simics/simulator/control.h>
#include <simics/simulator/memory.h>
#include "ccsimics/simics_util.h"
#include "crypto/cc_encoding.h"

enum class RW { READ, WRITE };
inline bool is_write(enum RW rw) { return rw == RW::WRITE; }

/**
 * @brief Base model for C3 heap pointer protections
 *
 * This implements Simics callbacks for address_before_cb, read_before_cb, and
 * write_before; which are trigger before an address is translated, and before
 * and after the memory has been read. The former intercepts any C3 encoded
 * linear addresses (i.e., a cryptographic address (CA)) and decodes them to
 * canonical linear addresses.  The latter two perform data encryption if the
 * original address used for the memory access was a CA.
 *
 * @tparam ConnectionTy A pointer to the SimicsConnection
 * @tparam CtxTy A pointer to the Context object
 * @tparam PtrEncTy A pointer to the PointerEncoding object
 */
template <typename ConnectionTy, typename CtxTy, typename PtrEncTy>
class C3BaseModel {
 protected:
    ConnectionTy *con_;
    CtxTy *ctx_;
    PtrEncTy *ptrenc_;

    logical_address_t la_encoded_ = 0;
    logical_address_t la_decoded_ = 0;
    bool is_encoded_pointer_ = false;
    bool is_crossing_page_first_ = false;
    bool is_crossing_page_second_ = false;
    bool is_page_fault_ = false;
    uint64 encoded_addr_callback_cnt_ = 0;
    uint64 total_read_cnt_ = 0;
    uint64 total_write_cnt_ = 0;
    uint64 encoded_read_cnt_ = 0;
    uint64 encoded_write_cnt_ = 0;
    uint64 total_addr_callback_cnt_ = 0;

 public:
    C3BaseModel(ConnectionTy *con, CtxTy *ctx, PtrEncTy *ptrenc)
        : con_(con), ctx_(ctx), ptrenc_(ptrenc) {}

    virtual inline ~C3BaseModel() = default;

    /**
     * @brief Register callbacks for simulation
     *
     * As this is registered via a C-interface, the object will be passed to
     * the callback as an opaque void * pointer. Consequently, the thandler
     * uses the provided HandlerTy, which should match the implementing class,
     * to statically cast the void * pointer back to the correct type.
     *
     * @tparam HandlerTy Type of handler, used to cast from void *
     * @param con
     */
    template <typename HandlerTy>
    inline void register_callbacks(ConnectionTy *con);

    /**
     * @brief Callback triggered before address translation
     *
     * @param la
     * @param cpu
     * @param handle
     * @return logical_address_t
     */
    inline logical_address_t address_before(logical_address_t la,
                                            address_handle_t *handle);

    /**
     * @brief Common data access callback
     *
     * This performs both encryption and decryption depending on access type.
     *
     * @param obj
     * @param cpu
     * @param mem
     * @param rw Specifies whether we're dealing with a read or write access
     */
    inline void modify_data_on_mem_access(memory_handle_t *mem, enum RW rw);

    /**
     * @brief Exception handler
     *
     * If this callback is registered, it will trigger a Simics breakpoint
     * on exception if the break_on_decode_fault Simics option is set.
     *
     * @param obj
     * @param cpu
     * @param eq_handle
     * @param unused
     */
    inline void exception_before(conf_object_t *obj, conf_object_t *cpu,
                                 exception_handle_t *eq_handle,
                                 lang_void *unused);

    /**
     * @brief
     *
     */
    virtual inline void print_stats();

    /**
     * @brief Decode address in CA format (override in subclass if needed)
     *
     * @param la
     * @return logical_address_t
     */
    virtual inline logical_address_t decode_pointer(logical_address_t la) {
        return ctx_->cc_enabled() ? ptrenc_->decode_pointer(la) : la;
    }

    /**
     * @brief Encode address in LA from to generate CA with given metadata
     *
     * @param la
     * @param metadata
     * @return logical_address_t
     */
    virtual inline logical_address_t encode_pointer(logical_address_t la,
                                                    ptr_metadata_t *metadata) {
        return ctx_->cc_enabled() ? ptrenc_->encode_pointer(la, metadata) : la;
    }

    virtual inline cpu_bytes_t
    encrypt_decrypt_data(ptr_metadata_t *metadata, logical_address_t data_tweak,
                         cpu_bytes_t bytes, uint8 *bytes_buffer) {
        return encrypt_decrypt_bytes(metadata, data_tweak, ctx_->get_dp_key(),
                                     bytes, bytes_buffer);
    }

    /**
     * @brief Check if memory access is valid
     *
     * This is invoked in the modify_data_on_mem_accss callback to check whether
     * the LA access using the given CA is valid.  This is invoked in the
     * modify_data_on_mem_accss callback after the check_memory_access has
     * passed. At that point we have the la_encoded_ and la_decoded_ variables
     * set for the current memory access, so they are safe to use.
     *
     * The base C3BaseModel does not do any explicit checking.
     *
     * @param cpu
     * @param la
     * @param rw
     * @param data
     * @param size
     * @return true
     * @return false
     */
    virtual inline bool check_memory_access(logical_address_t la, enum RW rw,
                                            const uint8_t *data, size_t size) {
        return true;
    }

    /**
     * @brief Get the data tweak object
     *
     * This is invoked in the modify_data_on_mem_accss callback, after calling
     * check_memory_access(), provided it returned succcess. The returned value
     * will be used as the data encryption/decryption tweak for the memory
     * access.
     *
     * The base C3BaseModel simply returns the provided address (which is a CA).
     *
     * @param address
     * @return uint64_t
     */
    virtual inline uint64_t get_data_tweak(logical_address_t address) {
        return (uint64_t)address;
    }

    inline bool debug_on() const { return this->con_->debug_on; }
    inline logical_address_t get_cc_la_decoded() const {
        return this->la_decoded_;
    }
    inline void set_cc_la_decoded(logical_address_t la) {
        this->la_decoded_ = la;
    }
    inline logical_address_t get_cc_la_encoded() const {
        return this->la_encoded_;
    }
    inline void set_cc_la_encoded(logical_address_t la) {
        this->la_encoded_ = la;
    }

    inline bool is_encoded_pointer() const { return this->is_encoded_pointer_; }
    inline void set_is_encoded_pointer(bool v) {
        this->is_encoded_pointer_ = v;
    }

 protected:
    /**
     * @brief Performs address tranlation
     *
     * @param la
     * @param access_type
     * @return uint64_t
     */
    inline uint64_t translate_la_to_pa(
            uint64_t la,
            access_t access_type = ((access_t)(Sim_Access_Read |
                                               Sim_Access_Write))) const {
        auto pa_block = con_->logical_to_physical(la, access_type);
        ASSERT_MSG(pa_block.valid != 0,
                   "ERROR: invalid la->pa translation. Breaking\n");
        return pa_block.address;
    }

    virtual inline bool should_print_data_modification(const enum RW rw) const {
        return debug_on();
    }

    inline void print_data_modification(const uint64_t data_tweak,
                                        const cpu_bytes_t &bytes,
                                        const cpu_bytes_t &bytes_mod) {
        SIM_printf("data_tweak: 0x%016lx\n", data_tweak);
        SIM_printf("bytes_orig (%lu) = ", bytes.size);
        for (int i = bytes.size - 1; i >= 0; i--) {
            SIM_printf("%02x ", bytes.data[i]);
        }
        SIM_printf("\n");
        SIM_printf("bytes_mod  (%lu) = ", bytes.size);
        for (int i = bytes_mod.size - 1; i >= 0; i--) {
            SIM_printf("%02x ", bytes_mod.data[i]);
        }
        SIM_printf("\n");
    }

    /**
     * @brief Handle mismatch between la_decoded and la of mem access
     *
     * @return true Mismatch is expected, and data modification should continue
     * @return false Stop mem access handling before data modification
     */
    inline virtual bool handle_la_mismatch_on_mem_access(logical_address_t la) {
        static int la_mismatch_count = 0;
        if (la_mismatch_count >= 3) {
            return false;
        }

        la_mismatch_count++;
        SIM_printf("******** WARNING: memory R/W to LA different from "
                   "decoded LA *************\n");
        SIM_printf("LA for this R/W: 0x%016lx\n", (uint64_t)la);
        SIM_printf("decoded LA     : 0x%016lx\n", (uint64_t)this->la_decoded_);
        if (this->is_crossing_page_first_) {
            SIM_printf("CROSSING_PAGE_FIRST");
            this->is_crossing_page_first_ = false;
        }
        if (this->is_crossing_page_second_) {
            SIM_printf("CROSSING_PAGE_SECOND");
            this->is_crossing_page_second_ = false;
        }
        this->is_encoded_pointer_ = false;
        return false;
    }
};

template <typename ConnectionTy, typename CtxTy, typename PtrEncTy>
template <typename HandlerTy>
void C3BaseModel<ConnectionTy, CtxTy, PtrEncTy>::register_callbacks(
        ConnectionTy *con) {
    /* Register a callback that will be called for each instruction */
    con_->register_address_before_cb(
            [](auto *obj, auto *cpu, auto la, auto *handle, auto *m) {
                return static_cast<HandlerTy *>(m)->address_before(la, handle);
            },
            static_cast<void *>(this));

    if (con_->disable_data_encryption) {
        SIM_printf("[CC] DATA ENCRYPTION DISABLED\n");
    }

    con_->register_read_before_cb(
            CPU_Access_Scope_Explicit,
            [](auto *obj, auto *cpu, auto *mem, auto *m) {
                static_cast<HandlerTy *>(m)->modify_data_on_mem_access(
                        mem, RW::READ);
            },
            static_cast<void *>(this));
    con_->register_read_before_cb(
            CPU_Access_Scope_Implicit,
            [](auto *obj, auto *cpu, auto *mem, auto *m) {
                static_cast<HandlerTy *>(m)->modify_data_on_mem_access(
                        mem, RW::READ);
            },
            static_cast<void *>(this));
    con_->register_write_before_cb(
            CPU_Access_Scope_Explicit,
            [](auto *obj, auto *cpu, auto *mem, auto *m) {
                static_cast<HandlerTy *>(m)->modify_data_on_mem_access(
                        mem, RW::WRITE);
            },
            static_cast<void *>(this));
    con_->register_write_before_cb(
            CPU_Access_Scope_Implicit,
            [](auto *obj, auto *cpu, auto *mem, auto *m) {
                static_cast<HandlerTy *>(m)->modify_data_on_mem_access(
                        mem, RW::WRITE);
            },
            static_cast<void *>(this));
}

template <typename ConnectionTy, typename CtxTy, typename PtrEncTy>
logical_address_t C3BaseModel<ConnectionTy, CtxTy, PtrEncTy>::address_before(
        logical_address_t la, address_handle_t *handle) {
    this->total_addr_callback_cnt_++;
    if (is_encoded_cc_ptr(la)) {
        this->is_encoded_pointer_ = true;
        this->encoded_addr_callback_cnt_++;
        if (debug_on()) {
            SIM_printf(
                    "******** CC address detected. Count: %lu *************\n",
                    (uint64_t)this->encoded_addr_callback_cnt_);
            SIM_printf("address_before : 0x%016lx\n", (uint64_t)la);
        }
        logical_address_t la_decoded = this->decode_pointer(la);
        if (is_canonical(la_decoded) != 0) {
            this->la_decoded_ = la_decoded;
            this->la_encoded_ = la;
            if (debug_on()) {
                SIM_printf("decoded address: 0x%016lx \n",
                           (uint64_t)la_decoded);
            }
        } else {
            static int non_canonical_count = 1;
            const int max_non_canonical_count = 3;
            if (non_canonical_count <= max_non_canonical_count) {
                SIM_printf("******** WARNING: non-canonical decoded address "
                           "*************\n");
                SIM_printf("address_before : 0x%016lx\n", (uint64_t)la);
                SIM_printf("decoded address: 0x%016lx\n", (uint64_t)la_decoded);
                SIM_printf("This can be benign if due to a SW prefetch\n");
                if (this->con_->break_on_decode_fault) {
                    SIM_printf("Stopping simulation due to warning. To proceed "
                               "at your own risk, press run.\n");
                    SIM_break_simulation("break_on_decode_fault: "
                                         "non-canonical address");
                }
                if (non_canonical_count == max_non_canonical_count) {
                    SIM_printf("Maximum warnings about non-canonical decoded "
                               "addres reached. Supressing further warnings\n");
                }
                non_canonical_count++;
            }
        }
        if (con_->get_page_crossing_info(handle) == Sim_Page_Crossing_First) {
            if (debug_on()) {
                SIM_printf("INFO: address_before: Sim_Page_Crossing_First\n");
                SIM_printf("address_before : 0x%016lx\n", (uint64_t)la);
                SIM_printf("decoded address: 0x%016lx\n", (uint64_t)la_decoded);
            }
            this->is_crossing_page_first_ = true;
        }
        return la_decoded;
    }
    if (this->is_crossing_page_second_) {
        // If here means that this is the second transaction from a cross-page
        // access using encoded pointer double-check that we are indeed crossing
        // page
        if (this->debug_on()) {
            SIM_printf("INFO: address_before: Sim_Page_Crossing_Second\n");
            SIM_printf("cc_la_encoded : 0x%016lx\n",
                       (uint64_t)this->la_encoded_);
            SIM_printf("cc_la_decoded : 0x%016lx\n",
                       (uint64_t)this->la_decoded_);
        }
        if (con_->get_page_crossing_info(handle) != Sim_Page_Crossing_Second) {
            SIM_printf("******** ERROR: something went wrong with cross-page "
                       "handling. Notify the owner please to fix! "
                       "*************\n");
            if (con_->break_on_decode_fault) {
                SIM_break_simulation("break_on_decode_fault: "
                                     "Cross-page handling failure");
            }
            this->con_->gp_fault(0, false, "Cross-page handling fail");
            ASSERT(0);
        }
        this->is_encoded_pointer_ =
                true;  // enforce that we are still processing transaction
                       // related to encoded pointer
        // no need to modify cc_la_encoded and cc_la_decoded as they are set in
        // modify_data callback
    } else {
        this->is_encoded_pointer_ = false;
    }
    return la;
}

template <typename ConnectionTy, typename CtxTy, typename PtrEncTy>
void C3BaseModel<ConnectionTy, CtxTy, PtrEncTy>::modify_data_on_mem_access(
        memory_handle_t *mem, enum RW rw) {
    if (!this->is_encoded_pointer_) {
        return;
    }

    auto la = con_->logical_address(mem);

    if (la == 0) {
        return;
    }

    auto bytes = con_->get_bytes(mem);
    uint8 bytes_buffer[64];

    // check for corner cses when no data is returned
    if (bytes.data == NULL || bytes.size == 0) {
        if (bytes.size == 0) {
            return;
        }
        if (bytes.size <= 64) {
            auto pa = con_->physical_address(mem);
            uint64_t data[8];
            for (size_t i = 0; i * 8 < bytes.size; i++) {
                data[i] = con_->read_phys_memory(pa + (i << 3), 8);
            }
            bytes.data = (const uint8 *)&data;
        } else {
            SIM_printf("ERROR: bytes.size =%ld which is > 64. CC data "
                       "decryption disabled for this memory access.\n",
                       bytes.size);
            ASSERT(0);
            // Raise fault in case asserts are disabled
            this->con_->gp_fault(0, false, "Internal error: Bad data size");
            return;
        }
    }
    // We expect that la_decoded_ from address_before matches with la here
    if (la != this->la_decoded_) {
        // If not, check if the mismatch is expected or can be fixed somehow
        if (!handle_la_mismatch_on_mem_access(la)) {
            // If not, stop here before data modification
            return;
        }
    }
    check_memory_access(la, rw, bytes.data, bytes.size);
    uint64_t data_tweak = get_data_tweak(this->la_encoded_);
    ptr_metadata_t cc_metadata = {0};
    cpu_bytes_t bytes_mod =
            encrypt_decrypt_data(&cc_metadata, data_tweak, bytes, bytes_buffer);

    if (!this->con_->disable_data_encryption) {
        con_->set_bytes(mem, bytes_mod);
    }
    if (should_print_data_modification(rw)) {
        print_data_modification(data_tweak, bytes, bytes_mod);
    }

    //  Handling Cross-Page Accesses
    if (this->is_crossing_page_second_) {
        // If here, means that we just finished the second chunk. Turn off
        // the flag
        if (debug_on()) {
            SIM_printf("INFO: modify_data: Sim_Page_Crossing_Second\n");
        }
        this->is_crossing_page_second_ = false;
    }

    if (this->is_crossing_page_first_) {
        if (debug_on()) {
            SIM_printf("INFO: modify_data: Sim_Page_Crossing_First\n");
        }
        // If here, means that we just finished the first chunk. Advance
        // la_encoded
        this->la_encoded_ = this->la_encoded_ + bytes.size;
        this->la_decoded_ = this->la_decoded_ + bytes.size;
        this->is_crossing_page_first_ = false;
        this->is_crossing_page_second_ = true;
    }
}

template <typename ConnectionTy, typename CtxTy, typename PtrEncTy>
void C3BaseModel<ConnectionTy, CtxTy, PtrEncTy>::exception_before(
        conf_object_t *obj, conf_object_t *cpu, exception_handle_t *eq_handle,
        lang_void *unused) {
    auto *con = static_cast<ConnectionTy *>(SIM_object_data(obj));
    if (con->break_on_decode_fault) {
        int exc_num = con->eq_iface->exception_number(cpu, eq_handle);
        if (exc_num < 32 && exc_num != 14) {
            SIM_printf("Breaking on exception #%d\n", exc_num);
            SIM_break_simulation("Break on exception");
        }
    }
}

template <typename ConnectionTy, typename CtxTy, typename PtrEncTy>
void C3BaseModel<ConnectionTy, CtxTy, PtrEncTy>::print_stats() {
    SIM_printf("Printing stats\n");
    SIM_printf("Total write count:   %llu\n", this->total_write_cnt_);
    SIM_printf("Total read count:   %llu\n", this->total_read_cnt_);
    SIM_printf("Encoded write count:   %llu\n", this->encoded_write_cnt_);
    SIM_printf("Encoded read count:   %llu\n", this->encoded_read_cnt_);
    SIM_printf("Total address callback count:   %llu\n",
               this->total_addr_callback_cnt_);
    SIM_printf("Encoded address callback count: %llu\n",
               this->encoded_addr_callback_cnt_);
    float ratio =
            100.0 * (static_cast<float>(this->encoded_addr_callback_cnt_) /
                     static_cast<float>(this->total_addr_callback_cnt_));
    SIM_printf("Encoded to total ratio:         %.2f%%\n", ratio);
}

#endif  // MODULES_COMMON_CCSIMICS_C3_BASE_MODEL_H_
