// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef MODULES_COMMON_CCSIMICS_INTEGRITY_H_
#define MODULES_COMMON_CCSIMICS_INTEGRITY_H_

#include "ccsimics/c3_base_model.h"
#include "ccsimics/icv_map.h"
#include "ccsimics/simics_connection.h"

#define INTEGRITY_FOR_OPTIONS(op)                                              \
    op(integrity_icv_reset, "reset ICV map");                                  \
    op(integrity_correct_on_fail, "integrity_correct_on_fail");                \
    op(integrity_break_on_write_mismatch,                                      \
       "integrity_break_on_write_mismatch");                                   \
    op(integrity_break_on_read_mismatch, "integrity_break_on_read_mismatch");  \
    op(integrity_fault_on_write_mismatch,                                      \
       "integrity_fault_on_write_mismatch");                                   \
    op(integrity_fault_on_read_mismatch, "integrity_fault_on_read_mismatch");  \
    op(integrity_warn_on_read_mismatch, "integrity_warn_on_read_mismatch");    \
    op(integrity_isa_fault_on_param_mismatch,                                  \
       "integrity_isa_fault_on_param_mismatch");

namespace ccsimics {

/** Modes for suppressing ICV failures
 * NONE: no suppression
 * STRICT: suppress fault on reads from intra-object tripwires
 * PERMISSIVE: suppress fault reads and writes from/ to intra-object tripwires
 */
enum INTEGRITY_SUPPRESS_MODE { NONE, STRICT, PERMISSIVE };

template <typename CcTy, typename ConTy, typename CtxTy> class Integrity final {
    using SelfTy = Integrity<CcTy, ConTy, CtxTy>;
    static const bool kTrace = false;
    // Unconditionally enable debug messages for this class
    static constexpr bool kDebugAlways = false;
    static constexpr bool kUsePhysicalMapForICVs = true;
    static constexpr bool kIgnoreICVBreakOnReadLA = false;

    CcTy *const cc_;
    ConTy *const con_;
    CtxTy *const ctx_;

    INTEGRITY_FOR_OPTIONS(DECLARE)

    ICVMap *icv_map_ = nullptr;
    size_t icv_map_size_ = 0;

    bool is_icv_failure_ = false;
    bool is_icv_correction_ = false;

    enum INTEGRITY_SUPPRESS_MODE integrity_suppress_mode_;

 public:
    Integrity(CcTy *cc, ConTy *con, CtxTy *ctx)
        : cc_(cc), con_(con), ctx_(ctx),
          integrity_fault_on_write_mismatch(true),
          integrity_fault_on_read_mismatch(false) {
        ASSERT(cc != nullptr && con != nullptr && ctx != nullptr);
    }

    virtual inline ~Integrity() { delete this->icv_map_; }

    ADD_DEFAULT_ACCESSORS(integrity_correct_on_fail)
    ADD_DEFAULT_ACCESSORS(integrity_break_on_write_mismatch)
    ADD_DEFAULT_ACCESSORS(integrity_break_on_read_mismatch)
    ADD_DEFAULT_ACCESSORS(integrity_fault_on_write_mismatch)
    ADD_DEFAULT_ACCESSORS(integrity_fault_on_read_mismatch)
    ADD_DEFAULT_ACCESSORS(integrity_warn_on_read_mismatch)
    ADD_DEFAULT_ACCESSORS(integrity_isa_fault_on_param_mismatch)

    /**
     * @brief Enable integrity, and if needed, allocate ICV map
     */
    inline void enable() {
        ASSERT(ctx_->get_icv_enabled());
        if (this->icv_map_ == nullptr) {
            this->icv_map_ = new ICVMap();
            this->ctx_->set_icv_lock(true);
        }
    }

    inline bool is_enabled() const { return ctx_->get_icv_enabled(); }

    /**
     * @brief Disable integrity (does not release ICV map)
     */
    inline void disable() { ASSERT(!ctx_->get_icv_enabled()); }

    inline void clear_icv_map() {
        if (this->icv_map_ != nullptr) {
            this->icv_map_->clear();
        }
    }

    inline attr_value_t get_integrity_icv_reset(void *,
                                                attr_value_t *val) const {
        return SIM_make_attr_boolean(false);
    }

    inline set_error_t set_integrity_icv_reset(void *, attr_value_t *val,
                                               attr_value_t *) {
        if (SIM_attr_boolean(*val))
            clear_icv_map();
        return Sim_Set_Ok;
    }

    inline bool icv_correct_ca_on_fail() const {
        return is_enabled() && this->integrity_correct_on_fail;
    }

    inline bool icv_isa_fault_param_mismatch() const {
        return is_enabled() && this->integrity_isa_fault_on_param_mismatch;
    }

    inline bool icv_fault_on_write_mismatch() const {
        return is_enabled() && this->integrity_fault_on_write_mismatch;
    }

    inline bool icv_fault_on_read_mismatch() const {
        return is_enabled() && this->integrity_fault_on_read_mismatch;
    }

    inline bool icv_break_on_write_mismatch() const {
        return is_enabled() && this->integrity_break_on_write_mismatch;
    }

    inline bool icv_break_on_read_mismatch() const {
        return is_enabled() && this->integrity_break_on_read_mismatch;
    }

    inline bool icv_warn_on_read_mismatch() const {
        return is_enabled() && this->integrity_warn_on_read_mismatch;
    }

    inline bool icv_ignore_write_mismatches() const {
        return !(icv_fault_on_write_mismatch() ||
                 icv_break_on_write_mismatch());
    }

    inline bool icv_ignore_read_mismatches() const {
        return !(icv_fault_on_read_mismatch() || icv_break_on_read_mismatch() ||
                 icv_warn_on_read_mismatch());
    }

    inline bool icv_ignore_all_mismatches() const {
        return icv_ignore_read_mismatches() && icv_ignore_write_mismatches();
    }

    inline bool icv_ignored_mismatch(const enum RW rw) const {
        return ((rw == RW::READ && icv_ignore_read_mismatches()) ||
                (rw == RW::WRITE && icv_ignore_write_mismatches()));
    }

    inline bool dbg() { return this->kDebugAlways || this->con_->debug_on; }

    /**
     * @brief Check if we've had a non-ignored ICV mismatch during this "cycle"
     */
    inline bool icv_mismatch_in_cycle(const enum RW rw) const {
        return (this->is_icv_failure_ && !icv_ignored_mismatch(rw));
    }

    /**
     * @brief Check if we've performed ICV correction in this "cycle"
     */
    inline bool is_icv_corrected() const { return this->is_icv_correction_; }

    /**
     * @brief Suppress GP for failing ICV checks
     */
    void set_suppress_icv_mode(INTEGRITY_SUPPRESS_MODE mode);

    /**
     * @brief Internal implementation of Setting ICV to INIT
     */
    void initICV(uint64_t ca);

    /**
     * @brief Internal implementation of Setting ICV to PREINIT
     */
    void preInitICV(uint64_t ca);

    /**
     * @brief Internal implementation of invalitdating of an ICV ISA
     */
    void invalidate_icv(uint64_t ca);

    /**
     * @brief Internal implementation of initializing memory location to value
     * and setting ICV of an ICV ISA
     */
    void initialize_icv(uint64_t ca, uint64_t val);

    /**
     * @brief Internal implementation of copying ICV map entries
     */
    void copyICVs(uint64_t ca_dst, uint64_t cs_src, size_t n, bool backwards);

    /**
     * @brief Entry point to ICV check workflow from modify_data_on_mem_access
     *
     * NOTE: if integrity is partially enabled during run, this callback may
     * need to run even if checks are disabled as this also leads to the ICV
     * initialization.
     *
     * @param mem Memory handle provided to the Simcis mem access callback
     * @param rw Is this a read or write callback
     */
    inline void check_integrity(memory_handle_t *mem, enum RW rw);

    /**
     * @brief Fixup pointer based on ICV associated with target LA
     */
    template <typename T> inline T do_icv_correction(const T ptr, uint64_t la) {
        return static_cast<T>(do_icv_correction(get_ca_t(ptr), la).uint64_);
    }

    /**
     * @brief Same as template variant, but with ca_t type parameters
     */
    inline ca_t do_icv_correction(const ca_t ptr, uint64_t la) {
        return _do_icv_correction(ptr, la);
    }

    /**
     * @brief Fixup pointer based on ICV in pointed-to memory
     *
     * If input is LA, will use it as target memory location for ICV lookup,
     * otherwise if CA, then this will attempt to decode the pointer. Note that
     * since this relies on the used CA, it will not be able to correct if the
     * CA encoding itself is corrupted and does not result in a correct LA.
     */
    template <typename T> inline T do_icv_correction(const T ptr) {
        if (!icv_correct_ca_on_fail()) {
            return ptr;
        }
        const auto la =
                is_encoded_cc_ptr(ptr) ? this->cc_->decode_pointer(ptr) : ptr;
        return do_icv_correction(ptr, la);
    }

 private:
    /**
     * @brief Get the icv index for LA
     *
     * @param address
     */
    inline uint64_t get_icv_index(const uint64_t la) const;

    /**
     * @brief Check ICV, return true if performed ICV correction
     */
    inline void set_icv(uint64_t icv_index, uint64_t ca, enum RW rw);

    /**
     * @brief Get ICV at index icv_index
     */
    inline icv_t get_icv(uint64_t icv_index) const;

    /**
     * @brief Check ICV, return true if performed ICV correction
     */
    inline bool check_icv(uint64_t icv_index, uint64_t ca, enum RW rw);

    /**
     * @brief Determine if ICV violation checking should be overridden
     */
    inline bool should_skip_icv_check(uint64_t ca, enum RW rw);

    /**
     * @brief Checks or set ICV for LA based on given CA
     *
     * Used for both check and set. If C3 is configured with ICV lock disabled
     * then the generated ICV for the CA is just set as the ICV, otherwise it
     * is checked against the in-memory ICV.
     */
    inline bool set_or_check_icv(logical_address_t la, uint64_t ca, enum RW rw);

    /**
     * @brief Check if C3-wide debug_on is enabled.
     */
    inline bool debug_on() const { return this->con_->debug_on; }

    /**
     * @brief Internal implementation of do_icv_correction
     */
    inline ca_t _do_icv_correction(const ca_t ca, uint64_t la);

    /**************************************************************************/
    /* Static functions *******************************************************/
    /**************************************************************************/

 public:
    /**
     * @brief Register inegirty attributes to the Simics connection
     */
    static inline void register_attributes(conf_class_t *cl);
};

template <typename CcTy, typename ConTy, typename CtxTy>
void Integrity<CcTy, ConTy, CtxTy>::set_suppress_icv_mode(
        INTEGRITY_SUPPRESS_MODE mode) {
    integrity_suppress_mode_ = mode;
}

template <typename CcTy, typename ConTy, typename CtxTy>
void Integrity<CcTy, ConTy, CtxTy>::invalidate_icv(uint64_t ptr) {
    // check ca
    const auto is_ca = is_encoded_cc_ptr(ptr);
    if (is_ca) {
        auto la = this->cc_->decode_pointer(ptr);
        const auto icv_index = get_icv_index(la);
        ASSERT_MSG(icv_index != ~0UL, "ICV fetch LA to PA translation failed");

        auto this_icv = is_ca ? (icv_t)(ptr & ICV_ADDR_MASK) : 0;
        auto stored_icv = get_icv(icv_index);

        if (this_icv == stored_icv) {
            // ICV's match expected ca allow invalidation
            this->icv_map_->set(icv_index, ~0);
        } else {
            // mismatched ICV,
            if (icv_fault_on_write_mismatch()) {
                SIM_printf("Fault on ICV fail\n");
                this->con_->gp_fault(0, false, "ICV fail");
            }
        }
    } else {
        SIM_printf("invalidate_icv: not a CA used for invalidation: (%lx).\n",
                   ptr);
        if (debug_on()) {
            SIM_break_simulation("break on non CA used for invicv.");
        }
        this->con_->gp_fault(0, false, "invalidate_icv");
    }
}

template <typename CcTy, typename ConTy, typename CtxTy>
void Integrity<CcTy, ConTy, CtxTy>::initialize_icv(uint64_t ca, uint64_t val) {
    // check ca
    const auto is_ca = is_encoded_cc_ptr(ca);
    if (is_ca) {
        auto la = this->cc_->decode_pointer(ca);
        const auto icv_index = get_icv_index(la);
        ASSERT_MSG(icv_index != ~0UL, "ICV fetch LA to PA translation failed");

        if (!this->con_->disable_data_encryption) {
            cpu_bytes_t bytes;
            bytes.size = 8;
            bytes.data = reinterpret_cast<uint8_t *>(&val);
            uint8 bytes_buffer[64];
            uint64_t data_tweak = this->cc_->get_data_tweak(ca);
            cpu_bytes_t bytes_mod = this->cc_->encrypt_decrypt_data(
                    nullptr, data_tweak, bytes, bytes_buffer);
            uint64_t enc_val =
                    *(reinterpret_cast<const uint64_t *>(bytes_mod.data));
            this->con_->write_mem_8B(la, enc_val);

        } else {
            this->con_->write_mem_8B(la, val);
        }

        set_icv(icv_index, ca, RW::WRITE);

    } else {
        SIM_printf("initialize_icv: not a CA used for invalidation: (%lx).\n",
                   ca);
        if (debug_on()) {
            SIM_break_simulation("break on non CA used for invicv.");
        }
        this->con_->gp_fault(0, false, "initialize_icv");
    }
}

template <typename CcTy, typename ConTy, typename CtxTy>
void Integrity<CcTy, ConTy, CtxTy>::copyICVs(uint64_t dst, uint64_t src,
                                             size_t n, bool backwards) {
    auto la_to_icv_i = [this](uint64_t la, size_t i, bool back) {
        const uint64_t icv_index =
                get_icv_index(back ? la - (i << SLOT_SIZE_IN_BITS)
                                   : la + (i << SLOT_SIZE_IN_BITS));
        ASSERT_MSG(icv_index != ~0UL, "ICV fetch LA to PA translation failed");
        return icv_index;
    };

    const auto la_dst =
            is_encoded_cc_ptr(dst) ? this->cc_->decode_pointer(dst) : dst;

    const auto la_src =
            is_encoded_cc_ptr(src) ? this->cc_->decode_pointer(src) : src;

    // Need to get number of ICV entries
    // N = (ALIGN_DOWN(src) - ALIGN_UP(src + n)) / SLOT_SIZE
    size_t aligned_src = la_src & (~0 << SLOT_SIZE_IN_BITS);
    size_t aligned_dst = la_dst & (~0 << SLOT_SIZE_IN_BITS);
    size_t aligned_src_end = (la_src + n + (1 << SLOT_SIZE_IN_BITS) - 1) &
                             (~0 << SLOT_SIZE_IN_BITS);
    size_t aligned_dst_end = (la_dst + n + (1 << SLOT_SIZE_IN_BITS) - 1) &
                             (~0 << SLOT_SIZE_IN_BITS);
    size_t src_icv_n = (aligned_src_end - aligned_src) >> SLOT_SIZE_IN_BITS;
    size_t dst_icv_n = (aligned_dst_end - aligned_dst) >> SLOT_SIZE_IN_BITS;

    auto dst_to_icv = [](uint64_t dst, size_t i, icv_t old_icv,
                         bool back) -> icv_t {
        if (!is_encoded_cc_ptr(dst))
            return 0ul;
        if (old_icv == ~0ul)
            return ~0ul;
        if (back) {
            return ((dst - (i << SLOT_SIZE_IN_BITS)) & ICV_ADDR_MASK);
        } else {
            return ((dst + (i << SLOT_SIZE_IN_BITS)) & ICV_ADDR_MASK);
        }
    };

    if (backwards) {
        size_t i;
        // Need to get number of ICV entries
        // N = (ALIGN_DOWN(src - n) - ALIGN_UP(src)) / SLOT_SIZE
        aligned_src = (la_src - n) & (~0 << SLOT_SIZE_IN_BITS);
        aligned_dst = (la_dst - n) & (~0 << SLOT_SIZE_IN_BITS);
        aligned_src_end = (la_src + (1 << SLOT_SIZE_IN_BITS) - 1) &
                          (~0 << SLOT_SIZE_IN_BITS);
        aligned_dst_end = (la_dst + (1 << SLOT_SIZE_IN_BITS) - 1) &
                          (~0 << SLOT_SIZE_IN_BITS);
        src_icv_n = (aligned_src_end - aligned_src) >> SLOT_SIZE_IN_BITS;
        dst_icv_n = (aligned_dst_end - aligned_dst) >> SLOT_SIZE_IN_BITS;
        for (i = 0; i < src_icv_n && i < dst_icv_n; i++) {
            auto val = this->icv_map_->get(la_to_icv_i(la_src, i, backwards));
            auto new_icv = dst_to_icv(dst, i, val, backwards);
            this->icv_map_->set(la_to_icv_i(la_dst, i, backwards), new_icv);
        }
        // Handle case where dst is unaligned -> has more ICV entries than src
        if (src_icv_n < dst_icv_n) {
            auto val = this->icv_map_->get(la_to_icv_i(la_src, 1, !backwards));
            auto new_icv = dst_to_icv(dst, i, val, backwards);
            this->icv_map_->set(la_to_icv_i(la_dst, i, backwards), new_icv);
            i++;
        }
        ifdbgprint(dbg(), "backwards copied %lu ICV entries (n = %lu)\n", i, n);
    } else {
        size_t i;
        for (i = 0; i < src_icv_n && i < dst_icv_n; i++) {
            auto val = this->icv_map_->get(la_to_icv_i(la_src, i, backwards));
            auto new_icv = dst_to_icv(dst, i, val, backwards);
            this->icv_map_->set(la_to_icv_i(la_dst, i, backwards), new_icv);
        }
        // Handle case where dst is unaligned -> has more ICV entries than src
        if (src_icv_n < dst_icv_n) {
            auto val = this->icv_map_->get(la_to_icv_i(la_src, 1, !backwards));
            auto new_icv = dst_to_icv(dst, i, val, backwards);
            this->icv_map_->set(la_to_icv_i(la_dst, i, backwards), new_icv);
            i++;
        }
        ifdbgprint(dbg(), "copied %lu ICV entries\n", i);
    }
}

template <typename CcTy, typename ConTy, typename CtxTy>
void Integrity<CcTy, ConTy, CtxTy>::preInitICV(uint64_t ptr) {
    // check ca
    const auto is_ca = is_encoded_cc_ptr(ptr);
    if (is_ca) {
        auto la = this->cc_->decode_pointer(ptr);
        const auto icv_index = get_icv_index(la);
        ASSERT_MSG(icv_index != ~0UL, "ICV fetch LA to PA translation failed");

        auto stored_icv = get_icv(icv_index);

        // Realloc changes the size of an allocation, without touching the
        // existing allocation, as stated in the manual page:
        //
        //     The realloc() function changes the size of the memory block
        //     pointed to by ptr to  size bytes.  The contents will be
        //     unchanged in the range from the start of the region up to the
        //     minimum of the old and new sizes.  If the new size is larger
        //     than  the  old  size, the added memory will not be initialized.
        //
        // If we set the entire allocation to PREINIT (and de-initialized it),
        // granules that have been previously initialized will loose their
        // initialization status. Thus, do not touch granules that already have
        // PREINIT set.
        if ((stored_icv & ICV_PREINIT_MASK) == ICV_PREINIT_MASK)
            return;

        // Set the PREINIT bit.
        ifdbgprint(dbg(), "Setting PREINIT bit.");
        stored_icv = stored_icv | ICV_PREINIT_MASK;

        // All memory allocations start with the least significant bits of the
        // ICV as zeros. Since the INIT bit has inverted polarity, i.e. since
        // zero means initialized, memory allocations start as initialized.
        // Thus, the PreInitICV instruction de-initializes the memory granule,
        // so that detection can occur.
        stored_icv = ICV_UNINITIALIZE(stored_icv);

        this->icv_map_->set(icv_index, stored_icv);
    } else {
        // XXX: REVIEW
        SIM_printf("preInitICV: not a CA: (%lx).\n", ptr);
        SIM_break_simulation("break on non CA used for PreInitICV.");
    }
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline void Integrity<CcTy, ConTy, CtxTy>::check_integrity(memory_handle_t *mem,
                                                           enum RW rw) {
    auto la = this->con_->logical_address(mem);
    this->cc_->set_cc_la_decoded(la);  // needed to keep track of current LA
    if (la == 0)
        return;
    auto bytes = this->con_->get_bytes(mem);
    if (bytes.size == 0)
        return;

    logical_address_t la_last_byte = (la + bytes.size) - 1;
    int number_of_overlapping_slots = (la_last_byte >> 3) - (la >> 3) + 1;

    bool icv_correction_performed = false;
    for (int i = 0; i < number_of_overlapping_slots; i++) {
        auto la_to_check = la + (i << SLOT_SIZE_IN_BITS);
        auto ca_to_check = this->cc_->is_encoded_pointer()
                                   ? this->cc_->get_cc_la_encoded() +
                                             (i << SLOT_SIZE_IN_BITS)
                                   : 0LU;
        bool icv_corrected_this_granule =
                set_or_check_icv(la_to_check, ca_to_check, rw);
        if (i == 0) {
            icv_correction_performed = icv_corrected_this_granule;
        } else {
            if (icv_correction_performed != icv_corrected_this_granule) {
                SIM_printf("[ICV_WARNING]: Access at 0x%016lx overlaps a "
                           "boundary between encrypted and unencrypted data, "
                           "which may result in partial data garbling.\n",
                           (uint64_t)la);
            }
        }
    }
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline uint64_t
Integrity<CcTy, ConTy, CtxTy>::get_icv_index(const uint64_t la) const {
    ASSERT_MSG(!is_encoded_cc_ptr(la), "CA passed to get_icv_index");

    if (!kUsePhysicalMapForICVs) {
        return (la >> SLOT_SIZE_IN_BITS);
    }
    const auto pa_block = con_->logical_to_physical(la, Sim_Access_Read);

    if (pa_block.valid == 0) {
        // This currently naively assumes that if an address cannot be
        // translated, then we are already dealing with a PA.
        return (uint64_t)(la >> SLOT_SIZE_IN_BITS);
    }
    return (uint64_t)(pa_block.address >> SLOT_SIZE_IN_BITS);
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline void Integrity<CcTy, ConTy, CtxTy>::set_icv(uint64_t icv_index,
                                                   uint64_t ca, enum RW rw) {
    if (rw == RW::READ)
        return;  // skip reads

    const bool is_ca = is_encoded_cc_ptr(ca);
    this->icv_map_->set(icv_index, (is_ca ? (ca & ICV_ADDR_MASK) : 0));

    if (is_ca && this->con_->debug_on) {
        SIM_printf("ICV: setting icv for addr=%016lx, icv=%016lx\n", ca,
                   this->icv_map_->get(icv_index));
    }
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline icv_t Integrity<CcTy, ConTy, CtxTy>::get_icv(uint64_t icv_index) const {
    ASSERT_MSG(this->icv_map_ != nullptr, "get_icv called, but map unset");
    ifdbgprint(kTrace && debug_on(), "Found ICV index %lu -> %lu", icv_index,
               this->icv_map_->get(icv_index));
    return this->icv_map_->get(icv_index);
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline bool
Integrity<CcTy, ConTy, CtxTy>::set_or_check_icv(logical_address_t la,
                                                uint64_t ca, enum RW rw) {
    if (!is_enabled()) {
        return false;
    }
    const auto icv_index = get_icv_index(la);
    if (icv_index == ~0UL) {
        // End here if we couldn't determine a valid ICV index
        return false;
    }
    if (this->ctx_->get_icv_lock() == true) {
        return this->check_icv(icv_index, ca, rw);
    } else {
        set_icv(icv_index, ca, rw);
        return false;
    }
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline bool Integrity<CcTy, ConTy, CtxTy>::should_skip_icv_check(uint64_t ca,
                                                                 enum RW rw) {
    // Early skip out on INTEGRITY_SUPPRESS_MODE::NONE
    if (integrity_suppress_mode_ == INTEGRITY_SUPPRESS_MODE::NONE) {
        return false;
    }

    ifdbgprint(dbg(), "SUPPRESS MODE: %d\n", integrity_suppress_mode_);

    // Only allow override for CAs
    const auto is_ca = is_encoded_cc_ptr(ca);
    if (!is_ca) {
        return false;
    }
    // CA aligned to granule size, so that we can read the magic value for the
    // granule
    auto granule_ca = ca & (~0 << SLOT_SIZE_IN_BITS);

    ifdbgprint(dbg(), "SUPPRESSION LOGIC (is_ca=%d ca=%lx, mode=%d)\n", is_ca,
               ca, integrity_suppress_mode_);
    ifdbgprint(dbg(), "orig la: %llx - la %% 8 = %llx\n",
               this->cc_->get_cc_la_decoded(),
               this->cc_->get_cc_la_decoded() & (~0 << SLOT_SIZE_IN_BITS));

    uint64_t encrypted_mem_val = this->con_->read_mem_8B(
            this->cc_->get_cc_la_decoded() & (~0 << SLOT_SIZE_IN_BITS));

    auto model = con_->get_model();
    auto mem_val = model->encrypt_decrypt_u64(granule_ca, encrypted_mem_val);
    bool is_intra_tripwire = mem_val == MAGIC_VAL_INTRA;
    if (integrity_suppress_mode_ == INTEGRITY_SUPPRESS_MODE::STRICT) {
        ifdbgprint(
                dbg(),
                "INTEGRITY_SUPPRESS_MODE::STRICT handling (mem_val = %08lx)\n",
                mem_val);
        if (rw == RW::READ && is_intra_tripwire) {
            // Skip ICV fail on READ of MAGIC_VAL_INTRA
            this->is_icv_failure_ = false;
            return true;
        }
    } else if (integrity_suppress_mode_ ==
               INTEGRITY_SUPPRESS_MODE::PERMISSIVE) {
        ifdbgprint(dbg(),
                   "INTEGRITY_SUPPRESS_MODE::PERMISSIVE handling (mem_val = "
                   "%08lx)\n",
                   mem_val);
        if (is_intra_tripwire) {
            // Skip ICV fail on READ or WRITE of MAGIC_VAL_INTRA
            this->is_icv_failure_ = false;
            return true;
        }
    }  // END suppression logic
    return false;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline bool Integrity<CcTy, ConTy, CtxTy>::check_icv(uint64_t icv_index,
                                                     uint64_t ca, enum RW rw) {
    this->is_icv_correction_ = false;

    if (!this->is_enabled()) {
        return false;
    }

    const auto is_ca = is_encoded_cc_ptr(ca);
    auto this_icv = is_ca ? (icv_t)(ca & ICV_ADDR_MASK) : 0;
    auto stored_icv = get_icv(icv_index);

    if ((ICV_ADDR_MASK & this_icv) == (ICV_ADDR_MASK & stored_icv)) {
        this->is_icv_failure_ = false;
    } else {
        static uint64_t warning_count = 0;

        // Handle ICV check suppression
        if (should_skip_icv_check(ca, rw)) {
            ifdbgprint(dbg(), "SKIP Check ICV\n");
            this->is_icv_failure_ = false;
            return false;
        }

        if (!icv_ignored_mismatch(rw)) {
            SIM_printf("ICV_WARNING[%ld]: ICV mismatch on %s to %llx, "
                       "this_icv=%lx != stored_icv=%lx (rip: 0x%016lx)\n",
                       ++warning_count, (rw != RW::READ ? "write" : "read"),
                       is_ca ? this->cc_->get_cc_la_encoded()
                             : this->cc_->get_cc_la_decoded(),
                       this_icv, stored_icv, this->con_->read_rip());
        }

        if (rw != RW::READ) {
            if (icv_break_on_write_mismatch()) {
                SIM_printf("Break on ICV fail\n");
                SIM_break_simulation("ICV fail");
            }
            if (icv_fault_on_write_mismatch()) {
                SIM_printf("Fault on write ICV fail\n");
                this->con_->gp_fault(0, false, "write ICV fail");
            }
        } else {
            if (icv_break_on_read_mismatch() && !kIgnoreICVBreakOnReadLA) {
                SIM_printf("Break on ICV fail\n");
                SIM_break_simulation("ICV fail");
            }
            if (icv_fault_on_read_mismatch()) {
                SIM_printf("Fault on read ICV fail\n");
                this->con_->gp_fault(0, false, "read ICV fail");
            }
        }
        this->is_icv_failure_ = true;

        if (icv_correct_ca_on_fail()) {
            const auto new_enc = do_icv_correction(icv_index, ca);
            this->cc_->set_is_encoded_pointer(is_encoded_cc_ptr(new_enc));
            this->cc_->set_cc_la_encoded(new_enc);
            return true;
        }
    }

    // PREINIT logic
    //
    // When the PREINIT flag is set, automatically initialize the granule on
    // memory writes.
    if (IS_ICV_PREINIT_ENABLED(stored_icv)) {
        if (rw == RW::WRITE) {
            stored_icv = ICV_INITIALIZE(stored_icv);
            this->icv_map_->set(icv_index, stored_icv);
        }
    }
    //
    // On memory reads, check that the INIT state is initialized.
    if (rw == RW::READ) {
        if (IS_ICV_UNINITIALIZED(stored_icv)) {
            SIM_printf("Read from Uninitialized memory at %lx.\n", ca);
            if (icv_break_on_read_mismatch() && !kIgnoreICVBreakOnReadLA)
                SIM_break_simulation("CWE457");
            if (icv_fault_on_read_mismatch())
                this->con_->gp_fault(0, false, "CWE457");
        }
    }

    return false;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline ca_t
Integrity<CcTy, ConTy, CtxTy>::_do_icv_correction(const ca_t ca,
                                                  const uint64_t la) {
    if (!is_enabled()) {
        return ca;
    }
    ASSERT(icv_correct_ca_on_fail());
    const bool is_ca = is_encoded_cc_ptr(ca);

    const auto icv_index = get_icv_index(la);
    if (icv_index == ~0UL) {
        // End here if we couldn't determine a valid ICV index
        return ca;
    }
    const auto this_icv = is_ca ? (icv_t)(ca.uint64_ & ICV_ADDR_MASK) : 0;
    const auto stored_icv = get_icv(icv_index);

    if (this_icv == stored_icv) {
        return ca;  // Stop here if we're not correcting anything
    }

    // Don't unset here since we may have multiple calls per instruction
    this->is_icv_correction_ = true;

    SIM_printf("ICV_WARNING: Correcting LA/CA on ICV mismatch "
               "(rip: 0x%016lx)\n",
               this->con_->read_rip());

    auto new_ptr = ca;

    if (stored_icv != 0) {
        new_ptr = get_ca_t(stored_icv);
        new_ptr.plaintext_ = ca.plaintext_;
    }
    SIM_printf("ICV_WARNING: Correction modified 0x%016lx to 0x%016lx\n",
               ca.uint64_, new_ptr.uint64_);
    return new_ptr;
}

template <typename CcTy, typename ConTy, typename CtxTy>
inline void
Integrity<CcTy, ConTy, CtxTy>::register_attributes(conf_class_t *cl) {
    // Define macro to avoid code duplication
#define INTEGRITY_ATTR_REGISTER_ACCESSOR(option, desc)                         \
    SIM_register_typed_attribute(                                              \
            cl, #option,                                                       \
            [](void *d, auto *obj, auto *idx) {                                \
                auto *con = reinterpret_cast<ConTy *>(SIM_object_data(obj));   \
                auto *self = reinterpret_cast<SelfTy *>(con->get_integrity()); \
                if (self)                                                      \
                    return self->get_##option(d, idx);                         \
                return SIM_make_attr_boolean(false);                           \
            },                                                                 \
            nullptr,                                                           \
            [](void *d, auto *obj, auto *val, auto *idx) {                     \
                auto *con = reinterpret_cast<ConTy *>(SIM_object_data(obj));   \
                auto *self = reinterpret_cast<SelfTy *>(con->get_integrity()); \
                if (self)                                                      \
                    return self->set_##option(d, val, idx);                    \
                return Sim_Set_Object_Not_Found;                               \
            },                                                                 \
            nullptr,                                                           \
            (attr_attr_t)((unsigned int)Sim_Attr_Optional |                    \
                          (unsigned int)Sim_Attr_Read_Only),                   \
            "b", NULL, "Enables " desc ". Default off.");

    INTEGRITY_FOR_OPTIONS(INTEGRITY_ATTR_REGISTER_ACCESSOR)

#undef INTEGRITY_ATTR_REGISTER_ACCESSOR
}

};  // namespace ccsimics

#endif  // MODULES_COMMON_CCSIMICS_INTEGRITY_H_
