/*
 Copyright 2024 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_COMMON_CCSIMICS_STACK_HARDENING_H_
#define MODULES_COMMON_CCSIMICS_STACK_HARDENING_H_

namespace ccsimics {

class StackHardening {
 public:
    StackHardening() = default;
    virtual ~StackHardening() = default;

    virtual inline void enable_callbacks() = 0;
    virtual inline void disable_callbacks() = 0;

    virtual inline bool check_memory_access(logical_address_t, uint64_t,
                                            enum RW) = 0;

    virtual inline uint64_t get_data_tweak(logical_address_t la) = 0;

    virtual inline bool is_encoded_sp(const uint64_t ptr) = 0;

    virtual inline uint64_t decode_sp(const uint64_t ptr) = 0;

    virtual inline logical_address_t
    decode_sp(const logical_address_t ptr) final {
        return decode_sp(static_cast<const uint64_t>(ptr));
    }
};

}  // namespace ccsimics

#endif  // MODULES_COMMON_CCSIMICS_STACK_HARDENING_H_
