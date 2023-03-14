/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/

#pragma once

#include <simics/processor/types.h>
#include "lim_simics_module.h"

#include "lim_simics_connection.h"

// This model's specific variables and includes
extern int trace_only;
extern int lim_no_encode;
#include "lim_ptr_encoding.h"

class lim_class {
 private:
    LimSimicsConnection *con_;
    logical_address_t la_encoded = 0;
    logical_address_t la_decoded = 0;
    bool is_encoded_pointer_ = false;
    bool is_shared_pointer = false;
    bool is_crossing_page_first = false;
    bool is_crossing_page_second_ = false;
    bool is_page_fault = false;
    uint64 encoded_addr_callback_cnt_ = 0;
    uint64 total_read_cnt_ = 0;
    uint64 total_write_cnt_ = 0;
    uint64 encoded_read_cnt = 0;
    uint64 encoded_write_cnt = 0;
    uint64 total_addr_callback_cnt_ = 0;

    uint8 ptr_tag;
    uint8 encoded_size;
    logical_address_t la_middle;
    logical_address_t la_meta;
    physical_address_t pa_meta;
    size_t meta_size;
    lim_decoded_metadata_t dec_meta;
    page_crossing_info_t page_crossing_type;
    uint64 metadata_reads_cnt;

    // internal functions:
    bool curr_instr_is_prefetch();
    bool is_encoded_addr_debug_en();
    void raise_page_fault_exception(logical_address_t la, access_t access_type);
    void print_access_info(logical_address_t la);
    uint64_t translate_la_to_pa(uint64_t la, access_t access_type);
    void read_from_linear_addr_samepage(logical_address_t la, void *rd_data,
                                        size_t size);
    void write_to_linear_addr_samepage(uint64_t la, const void *wr_data,
                                       size_t size);
    void write_to_linear_addr_crosspage(uint64_t la, const void *wr_data,
                                        size_t size);
    void read_from_linear_addr_crosspage(logical_address_t la, void *rd_data,
                                         size_t size);

 public:
    lim_class(LimSimicsConnection *con);  // mandatory constructor definition

    virtual void register_callbacks(LimSimicsConnection *con);

    logical_address_t address_before(logical_address_t la, conf_object *cpu,
                                     address_handle_t *handle);
    void modify_data_on_mem_access(conf_object_t *obj, conf_object_t *cpu,
                                   memory_handle_t *mem, const char *rw);
    void exception_before(conf_object_t *obj, conf_object_t *cpu,
                          exception_handle_t *eq_handle, lang_void *unused);
    void custom_model_init(LimSimicsConnection *con);
    void print_stats();

 private:
    inline uint64_t translate_la_to_pa(
            uint64_t la,
            access_t access_type = ((access_t)(Sim_Access_Read |
                                               Sim_Access_Write))) const {
        auto pa_block = con_->logical_to_physical(la, access_type);
        ASSERT_MSG(pa_block.valid != 0,
                   "ERROR: invalid la->pa translation. Breaking\n");
        return pa_block.address;
    }
};
typedef lim_class module_class;
