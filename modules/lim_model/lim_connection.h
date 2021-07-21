/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/

#ifndef CONNECTION_H
#define CONNECTION_H

#include <simics/model-iface/cpu-instrumentation.h>
#include <simics/model-iface/exception.h>
#include <simics/model-iface/processor-info.h>
#include <simics/arch/x86-instrumentation.h>
#include <simics/arch/x86.h>
#include "lim_ptr_encoding.h"
#define SUCCESS 0
#define FAIL 1

#define NON_CANONICAL_ADDR 0x8000000000000000

#define FOR_OPTIONS(op)                                                 \
        op(debug_on,              "enable debug messages")                    \
        op(break_on_decode_fault, "halt simulation when a non-canonical address is decoded")  \
        op(disable_meta_check,    "disable LIM tag and bound checks")        \
        op(break_on_exception,    "stop simulation on some exception")      \
        op(trace_only,              "disable skip over metadata. This is for tracing only")     
         

#define DECLARE(option, desc)                        \
        bool option;

#if defined(__cplusplus)
extern "C" {
#endif

struct lim_model_t;


typedef struct {
        conf_object_t obj;
        conf_object_t *cpu;
        struct lim_model *tracer;
        
        int pa_digits;
        int va_digits;
        unsigned id;
        const processor_info_v2_interface_t *pi_iface;
        const cpu_instrumentation_subscribe_interface_t *ci_iface;
        const x86_address_query_interface_t *x86_aq_iface;
        const cpu_instruction_query_interface_t *iq_iface;
        const cpu_memory_query_interface_t *mq_iface;
        const cpu_exception_query_interface_t *eq_iface;
        const x86_memory_query_interface_t *x86_mq_iface;
        const x86_exception_interface_t *x86_ex_iface;
        
        FOR_OPTIONS(DECLARE)
        strbuf_t last_line;
        uint64 data_cnt;
        uint64 instr_cnt;
        uint64 exc_cnt;
        uint8 encoded_size;
        uint8 ptr_tag;
        logical_address_t la_encoded;
        logical_address_t la_decoded;
        logical_address_t la_middle;
        logical_address_t la_meta;
        physical_address_t pa_meta;
        size_t meta_size;
        lim_decoded_metadata_t dec_meta;
        bool is_encoded_pointer;
        page_crossing_info_t page_crossing_type;
        bool is_crossing_page_first;
        bool is_crossing_page_second;
        bool is_page_fault;
        uint64 encoded_addr_callback_cnt;  
        uint64 total_addr_callback_cnt;
        uint64 metadata_reads_cnt;

        uint64 new_access;
        FILE *fp;

} connection_t;

FORCE_INLINE connection_t *
obj_to_con(conf_object_t *obj)
{
        return (connection_t *)obj;
}

void init_connection(void);
void configure_connection(connection_t *con);

extern conf_class_t *connection_class;


#if defined(__cplusplus)
}
#endif
#endif        
