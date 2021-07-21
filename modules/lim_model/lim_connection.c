/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/

#include <simics/simulator-api.h>
#include <simics/arch/x86.h>
#include <simics/model-iface/timing-model.h>
#include "lim_connection.h"
#include "lim_model.h"
int trace_only;

#ifndef DEV_DEBUG
#define DEV_DEBUG 0
#endif
#define CORRUPTED_ADDR 0x0
conf_class_t *connection_class;

static bool curr_instr_is_prefetch(connection_t *con, conf_object_t *cpu);

static void raise_page_fault_exception(connection_t *con, conf_object_t *cpu, logical_address_t la, x86_access_type_t access_type){
    if (curr_instr_is_prefetch(con, cpu)) {
        SIM_printf("Info: skipping spurious page fault from prefetch\n");
        return;
    }

    if (con->is_page_fault) {
        SIM_printf("WARNING: two consecutive page faults!\n");
		assert(0);
    }
    uint32_t ecode = (access_type & Sim_Access_Write) ? 0x6 : 0x4;

    // PF_fault does long jump, hence need to reset state 
    con->is_encoded_pointer = false;

    con->is_page_fault = true;
    if (con->debug_on) SIM_printf("\nPage fault la=0x%016llx, ecode=0x%x\n", la, ecode);
    con->x86_ex_iface->PF_fault(cpu, la, ecode); 
}

static logical_address_t
address_before(conf_object_t *obj, conf_object_t *cpu,
             logical_address_t la, address_handle_t *handle, void *connection_user_data)
{
    connection_t *con = obj_to_con(obj);

    if (is_encoded_address(la))
    {
        con->la_decoded =  lim_decode_pointer(la);
        con->encoded_size = get_encoded_size(la);
        con->la_meta = get_metadata_address(con->la_decoded, con->encoded_size);
        con->meta_size = get_metadata_size(con->encoded_size);
        if (con->trace_only) {
            con->encoded_addr_callback_cnt++;
            if (con->debug_on){
                SIM_printf("******** LIM address detected. Count: %ld *************\n", (long int) con->encoded_addr_callback_cnt);
                SIM_printf("  address_before : 0x%016llx\n", (long long unsigned int) la);
                SIM_printf("  decoded address: 0x%016llx \n", (long long unsigned int) con->la_decoded);
                SIM_printf("  la_meta:         0x%016llx \n", (long long unsigned int) con->la_meta);
                SIM_printf("  meta_size:       %d\n", (int) con->meta_size);
            }
            return con->la_decoded;
        }

        con->is_encoded_pointer = true;
        uint64_t return_la = con->la_decoded;
        con->la_encoded = la;
        con->ptr_tag = get_encoded_tag(la);
        con->la_middle = get_middle_address(con->la_decoded, con->encoded_size);
        // Get metadata
        uint64_t metadata[LIM_METADATA_SIZE_512B/8];
        uint64_t metadata_pa[LIM_METADATA_SIZE_512B/8];
        physical_block_t pa_meta_block;
        for (size_t i = 0; i*8 < con->meta_size; i++) {
            uint64_t la_to_translate = con->la_meta + i*8;
            pa_meta_block = con->pi_iface->logical_to_physical(cpu, la_to_translate, Sim_Access_Read);
            if (pa_meta_block.valid==0) {
                raise_page_fault_exception(con, cpu, la_to_translate, Sim_Access_Read);
            } else {
                con->is_page_fault = false;
                metadata_pa[i] = pa_meta_block.address;
                metadata[i]=SIM_read_phys_memory(cpu, metadata_pa[i], 8);
            }
        }

        con->pa_meta = metadata_pa[0];
        con->dec_meta = lim_decode_metadata(metadata, con->meta_size, con->la_middle);
        con->page_crossing_type = con->x86_aq_iface->get_page_crossing_info(cpu, handle);
        con->encoded_addr_callback_cnt++;
        if (con->debug_on){
            SIM_printf("******** Tagged address detected. Count: %ld *************\n", (long int) con->encoded_addr_callback_cnt);
            SIM_printf("address_before : 0x%016llx\n", (long long unsigned int) la);
            SIM_printf("decoded address: 0x%016llx \n", (long long unsigned int) con->la_decoded);
            SIM_printf("la_meta:         0x%016llx \n", (long long unsigned int) con->la_meta);
            SIM_printf("pa_meta:         0x%016llx \n", (long long unsigned int) con->pa_meta);
            SIM_printf("meta_size:       %d\n", (int) con->meta_size);
            SIM_printf("ptr tag:         0x%x\n", con->ptr_tag);
            SIM_printf("ptr size enc:    0x%x (= %ld bytes)\n",con->encoded_size, get_slot_size_in_bytes(con->encoded_size));
            SIM_printf("Lower bound stored in metadata:              0x%016llx\n", (long long unsigned int) con->dec_meta.lower_la);
            SIM_printf("Upper bound stored in metadata:              0x%016llx\n", (long long unsigned int) con->dec_meta.upper_la);
            if (con->page_crossing_type == Sim_Page_Crossing_None) {
                SIM_printf("con->page_crossing_type == Sim_Page_Crossing_None\n");
            } else if (con->page_crossing_type == Sim_Page_Crossing_First) {
                SIM_printf("con->page_crossing_type == Sim_Page_Crossing_First\n");
            } else {
				assert(0);
            }
        }

        if (!con->disable_meta_check) {
            //Check if tags match
            if (con->ptr_tag != con->dec_meta.tag_left ||
                con->dec_meta.tag_left != con->dec_meta.tag_right) {
                SIM_printf("LIM WARNING: tag mismatch! ptr_tag=%x meta_tag=(%x,%x)\n", con->ptr_tag, con->dec_meta.tag_left, con->dec_meta.tag_right);
                SIM_printf("Intentionally corrupting pointer to 0x%016llx\n", (long long unsigned int)CORRUPTED_ADDR);
                return_la = CORRUPTED_ADDR;
            }
            // Check if lower/upper la are within the slot
            if (con->dec_meta.lower_la < get_slot_start(con->la_decoded, con->encoded_size) ||
                con->dec_meta.upper_la > get_slot_end(con->la_decoded, con->encoded_size) ) {
                SIM_printf("LIM WARNING: invalid boundary for encoded la=0x%016llx\n", (long long unsigned int) la);
                SIM_printf("Lower bound stored in metadata:              0x%016llx\n", (long long unsigned int) con->dec_meta.lower_la);
                SIM_printf("  Must be no lower than slot start:          0x%016llx\n", (long long unsigned int) get_slot_start(con->la_decoded, con->encoded_size));
                SIM_printf("Upper bound stored in metadata:              0x%016llx\n", (long long unsigned int) con->dec_meta.upper_la);
                SIM_printf("  Must be no greater than slot end:          0x%016llx\n", (long long unsigned int) get_slot_end(con->la_decoded, con->encoded_size));
                SIM_printf("Intentionally corrupting pointer to 0x%016llx\n", (long long unsigned int)CORRUPTED_ADDR);
                return_la = CORRUPTED_ADDR;
            }
        }

        if (con->x86_aq_iface->get_page_crossing_info(cpu, handle)==Sim_Page_Crossing_First){
            if (con->debug_on) SIM_printf("INFO: address_before: Sim_Page_Crossing_First\n");
            con->is_crossing_page_first = true;
        }

        // Check if the first byte is OOB and corrupt pointer.
        // The rest of the access will be checked in the subsequent callback for the data access.
        if (!con->disable_meta_check) {
            if (con->la_decoded < con->dec_meta.lower_la) {
                SIM_printf("LIM WARNING: OOB: crossing lower boundary for la(decoded)=0x%016llx\n", (long long unsigned int) con->la_decoded);
                SIM_printf("Allowed lower LA:                                         0x%016llx\n", (long long unsigned int) con->dec_meta.lower_la);
                SIM_printf("Intentionally corrupting pointer to 0x%016llx\n", (long long unsigned int)CORRUPTED_ADDR);
                return_la = CORRUPTED_ADDR;
            } else if (con->la_decoded > con->dec_meta.upper_la) {
                SIM_printf("LIM WARNING: OOB: crossing upper boundary for la(decoded)=0x%016llx\n", (long long unsigned int) con->la_decoded);
                SIM_printf("Allowed upper LA:                                         0x%016llx\n", (long long unsigned int) con->dec_meta.upper_la);
                SIM_printf("Intentionally corrupting pointer to 0x%016llx\n", (long long unsigned int)CORRUPTED_ADDR);
                return_la = CORRUPTED_ADDR;
            }
        }
        return return_la;
    }
    if (con->is_encoded_pointer && con->x86_aq_iface->get_page_crossing_info(cpu, handle)==Sim_Page_Crossing_Second) {
        // If here means that this is the second transaction from a cross-page access using encoded pointer
        con->page_crossing_type = Sim_Page_Crossing_Second;
        con->la_decoded = lim_decode_pointer(la);
        if (con->debug_on) SIM_printf("INFO: address_before: Sim_Page_Crossing_Second\n");
        if (con->debug_on) SIM_printf("la :         0x%016llx\n", (long long unsigned int) la);
        if (con->debug_on) SIM_printf("la_decoded : 0x%016llx\n", (long long unsigned int) con->la_decoded);
    }
    if (con->is_encoded_pointer) {
        if (con->debug_on) SIM_printf("WARNING (addr callback) con->is_encoded_pointer set. la : 0x%016llx\n", (long long unsigned int) la);
    }
    return la;


}

FORCE_INLINE size_t calc_bytes_before_pg_bndry (uint64_t la, size_t size) {
    uint64_t distance_to_page_bndry =SIM_PAGE_SIZE - (la & LIM_FMASK(PAGE_OFFSET));
    assert (distance_to_page_bndry <=SIM_PAGE_SIZE);
    return (size_t) min_uint64(distance_to_page_bndry, size);
}

static uint64_t translate_la_to_pa(connection_t* con, conf_object_t *cpu, uint64_t la, x86_access_type_t access_type) {
    physical_block_t pa_block;
    pa_block = con->pi_iface->logical_to_physical(cpu, la, access_type);
    if (pa_block.valid==0) {
        raise_page_fault_exception(con, cpu, la, access_type);
    } else {
        con->is_page_fault = false;
    }
    return pa_block.address;
}

static bool write_to_linear_addr_samepage(connection_t* con, conf_object_t *cpu, uint64_t la, const void* wr_data, size_t size) {
    if (get_page_addr(la) !=get_page_addr(la+size-1) ) {
        SIM_printf("%s:%d WARNING: get_page_addr(la) !=get_page_addr(la+size-1)\n", __func__, __LINE__);
        SIM_printf("la = 0x%016llx\n", (long long unsigned int) la);
        SIM_printf("size=%ld\n", size);
        SIM_printf("con->encoded_addr_callback_cnt=%lld\n", con->encoded_addr_callback_cnt);
		assert(0);
    }
    uint64_t pa = translate_la_to_pa(con, cpu, la, Sim_Access_Write);
    if (pa == 0) {
        SIM_printf("Failed to translate LA %p for write.\n", (void *)la);
		assert(0);
    }
    int remaining_bytes = (int) size;
    uint64_t* buf = (uint64_t*) wr_data;
    while (remaining_bytes > 0) {
        int wr_len = min_int(remaining_bytes, 8);
        SIM_write_phys_memory(cpu, pa, *buf, wr_len);
        if (SIM_get_pending_exception() != SimExc_No_Exception) {
            SIM_printf("%s:%d: Exception while writing %d bytes of physical memory @ %p (%s).\n", __FILE__, __LINE__, wr_len, (void *)pa, SIM_last_error());
            SIM_clear_exception();
			assert(0);
        }
        buf++;
        pa += 8;
        remaining_bytes -= 8;
    }
    return SUCCESS;
}

static void read_from_linear_addr_samepage(connection_t* con, conf_object_t *cpu, logical_address_t la, void* rd_data, size_t size) {
    if (get_page_addr(la) !=get_page_addr(la+size-1) ) {
        SIM_printf("%s:%d WARNING: get_page_addr(la) !=get_page_addr(la+size-1)\n", __func__, __LINE__);
        SIM_printf("la = 0x%016llx\n", (long long unsigned int) la);
        SIM_printf("size=%ld\n", size);
        SIM_printf("con->encoded_addr_callback_cnt=%lld\n", con->encoded_addr_callback_cnt);
    }
    uint64_t pa = translate_la_to_pa(con, cpu, la, Sim_Access_Read);
    if (pa == 0) {
        SIM_printf("Failed to translate LA %p for read.\n", (void *)la);
		assert(0);
    }
    int remaining_bytes= (int) size;
    uint64_t* buf = (uint64_t*) rd_data;
    while (remaining_bytes > 0) {
        int rd_len = min_int(remaining_bytes, 8);
        *buf = SIM_read_phys_memory(cpu, pa, rd_len);
        if (SIM_get_pending_exception() != SimExc_No_Exception) {
            SIM_printf("%s:%d: Exception while reading %d bytes of physical memory @ %p (%s).\n", __FILE__, __LINE__, rd_len, (void *)pa, SIM_last_error());
            SIM_clear_exception();
			assert(0);
        }
        buf++;
        pa += 8;
        remaining_bytes -= 8;
    }
}

static void write_to_linear_addr_crosspage(connection_t* con, conf_object_t *cpu, uint64_t la, const void* wr_data, size_t size) {
    assert (size <= 64);
    uint64_t bytes_before = calc_bytes_before_pg_bndry(la, size);
    assert (bytes_before <= size);
    int bytes_after = size - bytes_before;
    assert (bytes_after <= (int) size);
    assert (bytes_before + bytes_after == (int) size);

    write_to_linear_addr_samepage(con, cpu, la, wr_data, bytes_before);
    if (bytes_after){
        write_to_linear_addr_samepage(con, cpu, la+bytes_before, (uint8_t*)wr_data+bytes_before, bytes_after);
    }
}
static void read_from_linear_addr_crosspage(connection_t* con, conf_object_t *cpu, logical_address_t la, void* rd_data, size_t size) {
    assert (size <= 64);
    uint8_t buffer[64+8];
    uint64_t bytes_before = calc_bytes_before_pg_bndry(la, size);
    size_t bytes_after = size - bytes_before;
    assert (bytes_before <= size);
    assert (bytes_after <= (int) size);
    assert (bytes_before + bytes_after == (int) size);

    read_from_linear_addr_samepage(con, cpu, la, &buffer[0], bytes_before);
    if (bytes_after){
        read_from_linear_addr_samepage(con, cpu, la+bytes_before, &buffer[bytes_before], bytes_after);
    }
    memcpy(rd_data, buffer, size);
}

bool curr_instr_is_prefetch(connection_t *con, conf_object_t *cpu) {
    uint64 instr_la = con->pi_iface->get_program_counter(cpu);
    uint16_t instr_buf;
    read_from_linear_addr_crosspage(con, cpu, instr_la, &instr_buf, sizeof(instr_buf));
    return instr_buf == 0x180f; // Prefetch opcode
}

static void
modify_data_on_mem_access(conf_object_t *obj, conf_object_t *cpu,
           memory_handle_t *mem, const char *rw,
           void *connection)  
{
    connection_t *con = obj_to_con(obj);
    const cpu_memory_query_interface_t *mq = con->mq_iface;
    uint64 la = mq->logical_address(cpu, mem);
    cpu_bytes_t bytes = mq->get_bytes(cpu, mem);
    unsigned size = mq->get_bytes(cpu, mem).size;


    if (con->is_encoded_pointer && la ==0 ) {
        if (con->debug_on) SIM_printf("(%d) WARNING (data callback) con->is_encoded_pointer  with la : 0x%016llx\n\n", __LINE__, (long long unsigned int) la);
        return;
    }
    con->total_addr_callback_cnt++;
    if (con->is_encoded_pointer && la !=0)
    {
        if(con->debug_on) SIM_printf("data callback la=0x%016llx\n", (long long unsigned int) la);
        if (la != con->la_decoded) {
            if(con->debug_on) SIM_printf("(%d) WARNING (data callback) con->is_encoded_pointer  with la=0x%016llx != la_decoded=0x%016llx\n\n", __LINE__, la, con->la_decoded);
            con->is_encoded_pointer = false;
            return;
        }
        uint64_t la_last_byte = la+size-1;
        uint64_t la_last_byte_after_shift = la_last_byte + con->meta_size;
        bool transactionIsBeforeMeta = (la_last_byte < con->la_meta) ? true : false; 
        bool transactionIsAfterMeta = (la >= con->la_meta) ? true : false; 
        bool transactionIsOverMeta = (!transactionIsBeforeMeta && !transactionIsAfterMeta);
        if (! transactionIsBeforeMeta) {
            if (get_page_addr(la) != get_page_addr(la_last_byte_after_shift)) {
                //bytes shifted into the next page. Check if the translation for the next page is valid. If invalid, it will generate a page fault
                translate_la_to_pa(con, cpu,
                                    (la_last_byte_after_shift & 0xFFFFFFFFFFFFF000),
                                    (rw[0] == 'W') ? Sim_Access_Write : Sim_Access_Read);
            } else {
                con->is_page_fault = false;
            }
        }

        con->page_crossing_type = mq->get_page_crossing_info(cpu, mem); // This updates page crossing info in case it was changed due to address shift
        // Trivial case: zero bytes. Just return
        if (bytes.size == 0) {
            con->is_encoded_pointer = false;
            //if (con->debug_on) SIM_printf("con->is_encoded_poiner = 0 (%d)\n", __LINE__);
            return;
        }
        if (bytes.data == NULL) {
            //need to handle this case for trace-only. Return for now
            con->is_encoded_pointer = false;
            //if (con->debug_on) SIM_printf("con->is_encoded_poiner = 0 (%d)\n", __LINE__);
            return;
        }
        if(con->debug_on) 
        {
            SIM_printf("bytes_orig (%s %dB) = ", rw, (int) bytes.size);
            for (int i = bytes.size - 1; i >= 0; i--)
            {
                SIM_printf("%02x ", bytes.data[i]);
            }
            SIM_printf("\n"); 
        }
        // Check lower bound
        if (!con->disable_meta_check) {
            if (la < con->dec_meta.lower_la) {
                SIM_printf("LIM WARNING: OOB: crossing lower boundary for la=0x%016llx\n", (long long unsigned int) la);
                SIM_printf("Allowed lower LA:                                0x%016llx\n", (long long unsigned int) con->dec_meta.lower_la);
                if (con->break_on_decode_fault) SIM_printf("Breaking Simulation (line %d). Type run to continue...\n", __LINE__);
                if (con->break_on_decode_fault) SIM_break_simulation("");
            }
        }

        uint8_t fixup_data[64+LIM_METADATA_SIZE_512B];
        if (transactionIsAfterMeta){
            // Transaction after metadata
            if (rw[0] == 'R') {
                read_from_linear_addr_crosspage(con, cpu, la+con->meta_size, fixup_data, bytes.size);
            } else {
                read_from_linear_addr_crosspage(con, cpu, la, fixup_data, min_uint64(bytes.size, con->meta_size));
                write_to_linear_addr_crosspage(con, cpu, la+con->meta_size, bytes.data, bytes.size);
                if (bytes.size > con->meta_size) memcpy(&fixup_data[con->meta_size], bytes.data, bytes.size-con->meta_size);
            }
            cpu_bytes_t bytes_mod;
            bytes_mod.size = bytes.size;
            bytes_mod.data = (uint8_t*) fixup_data;
            mq->set_bytes(cpu, mem, bytes_mod);
            if(con->debug_on) {
                SIM_printf("bytes_mod  (%s %dB) = ", rw, (int) bytes_mod.size);
                for (int i = bytes_mod.size - 1; i >= 0; i--)
                {
                    SIM_printf("%02x ", bytes_mod.data[i]);
                }
                SIM_printf("\n"); 
            }
        }

        if (transactionIsOverMeta) {
            // Transaction over the metadata
            size_t bytes_before = con->la_meta - la;
            size_t bytes_after = bytes.size - bytes_before;
            assert (bytes_before!=0 && bytes_before < bytes.size);
            assert (bytes_after!=0 && bytes_after < bytes.size);
            if (rw[0] == 'R') {
                read_from_linear_addr_samepage(con, cpu, la,                          &fixup_data[0],            bytes_before);
                read_from_linear_addr_crosspage(con, cpu, con->la_meta+con->meta_size, &fixup_data[bytes_before], bytes_after);
            } else if (rw[0] == 'W') {
                //Write
                memcpy(fixup_data, bytes.data, bytes_before);
                read_from_linear_addr_crosspage(con, cpu, con->la_meta, &fixup_data[bytes_before], con->meta_size);
                memcpy(&fixup_data[bytes_before+con->meta_size], &bytes.data[bytes_before], bytes_after);
                uint64_t la_after_meta = con->la_meta + con->meta_size;
                write_to_linear_addr_crosspage(con, cpu, la_after_meta, &bytes.data[bytes_before], bytes_after);
            } else assert(0); // unknown rw[0] value
            cpu_bytes_t bytes_mod;
            bytes_mod.size = bytes.size;
            bytes_mod.data = (uint8_t*) fixup_data;
            mq->set_bytes(cpu, mem, bytes_mod);
        }

        // Checking upper bound
        if (!con->disable_meta_check) {
            if (la_last_byte_after_shift > con->dec_meta.upper_la) {
                SIM_printf("LIM WARNING: OOB: crossing upper boundary for la=0x%016llx\n", (long long unsigned int) la);
                SIM_printf("Transaction size:                                  %d\n", (int) size);
                SIM_printf("Last byte LA:                                    0x%016llx\n", (long long unsigned int) la_last_byte);
                SIM_printf("Allowed upper LA:                                0x%016llx\n", (long long unsigned int) con->dec_meta.upper_la);
                if (con->break_on_decode_fault) SIM_printf("Breaking Simulation (line %d). Type run to continue...\n", __LINE__);
                if (con->break_on_decode_fault) SIM_break_simulation("");
            }
        }


        //           Resetting Flags
        if (con->page_crossing_type == Sim_Page_Crossing_None || con->page_crossing_type == Sim_Page_Crossing_Second) {
            con->is_encoded_pointer      = false;
            con->is_crossing_page_second = false;
        }
            
        if (con->page_crossing_type == Sim_Page_Crossing_First) {
            con->page_crossing_type = Sim_Page_Crossing_Second;
            con->is_crossing_page_first  = false;
            con->is_crossing_page_second = true;
        }
        if(con->debug_on) SIM_printf("\n");
    } 
    return;
}

static void
read_forall(conf_object_t *obj, conf_object_t *cpu,
            memory_handle_t *mem, void *connection_user_data)
{
    modify_data_on_mem_access(obj, cpu, mem, "Read ", connection_user_data);
}

static void
write_forall(conf_object_t *obj, conf_object_t *cpu,
             memory_handle_t *mem, void *connection_user_data)
{
    modify_data_on_mem_access(obj, cpu, mem, "Write", connection_user_data);
}

static void
exception_before(conf_object_t *obj, conf_object_t *cpu,
                    exception_handle_t *eq_handle,
                    lang_void *unused)
{
    connection_t *con = obj_to_con(obj);
    if (con->break_on_exception) {
        int exc_num = con->eq_iface->exception_number(cpu, eq_handle);
        if (exc_num <32 && exc_num != 14) {
            SIM_printf("Breaking on exception #%d\n", exc_num);
            SIM_break_simulation(" ");
        }
    }
}

void
configure_connection(connection_t *con)
{
    /* Register a callback that will be called for each instruction */
    con->ci_iface->register_exception_before_cb(con->cpu, &con->obj, CPU_Exception_All,
            exception_before, NULL);
    con->ci_iface->register_address_before_cb(
            con->cpu, &con->obj, 
            address_before, NULL);
    if (con->trace_only) {
        SIM_printf("[LIM] Disabling skip over metadata for trace-only config\n");
        trace_only = 1;
    } else {
        con->ci_iface->register_read_before_cb(
                con->cpu, &con->obj, CPU_Access_Scope_Explicit,
                read_forall, NULL);
        con->ci_iface->register_write_before_cb(
                con->cpu, &con->obj, CPU_Access_Scope_Explicit,
                write_forall, NULL);
        trace_only = 0;
    }
    if(con->debug_on){
		SIM_printf("DEBUG messages enabled\n");
	}
}


static conf_object_t *
ic_alloc(void *arg)
{
    connection_t *con = MM_ZALLOC(1, connection_t);
    con->la_decoded = NON_CANONICAL_ADDR;
    con->is_encoded_pointer = false;
    con->is_crossing_page_first = false;
    con->is_crossing_page_second = false;
    con->is_page_fault = false;
    con->encoded_addr_callback_cnt = 0;
    con->total_addr_callback_cnt = 0;
    con->metadata_reads_cnt = 0;
    SIM_printf ("[CC] LIM Model enabled.\n");
    #ifdef PAGING_5LVL
    SIM_printf ("[CC] Using 5-level paging.\n");
    #endif

    return &con->obj;
}

// instrumentation_connection::pre_delete_instance
static void
ic_pre_delete_connection(conf_object_t *obj)
{
    connection_t *con = obj_to_con(obj);
    con->ci_iface->remove_connection_callbacks(con->cpu, &con->obj);
}

static int
ic_delete_connection(conf_object_t *obj)
{
    MM_FREE(obj);
    return 0;
}

// instrumentation_connection::enable
static void
ic_enable(conf_object_t *obj)
{
    connection_t *c = obj_to_con(obj);
    c->ci_iface->enable_connection_callbacks(c->cpu, obj);
}

static void
ic_disable(conf_object_t *obj)
{
    connection_t *c = obj_to_con(obj);
    c->ci_iface->disable_connection_callbacks(c->cpu, obj);
}

typedef enum {
        Data_Cache,
        Instr_Cache,
} di_t;


static attr_value_t
get_cpu(void *_id, conf_object_t *obj, attr_value_t *idx)
{
        connection_t *con = obj_to_con(obj);
        return SIM_make_attr_object(con->cpu);
}

#define MAKE_ATTR_SET_GET_BOOL(option, desc)                            \
static set_error_t                                                      \
set_ ## option (void *arg, conf_object_t *obj, attr_value_t *val,       \
              attr_value_t *idx)                                        \
{                                                                       \
    connection_t *con = obj_to_con(obj);                            \
    con-> option = SIM_attr_boolean(*val);                          \
    return Sim_Set_Ok;                                              \
}                                                                       \
                                                                        \
static attr_value_t                                                     \
get_ ## option (void *arg, conf_object_t *obj, attr_value_t *idx)       \
{                                                                       \
    connection_t *con = obj_to_con(obj);                            \
    return SIM_make_attr_boolean(con-> option );                    \
}

FOR_OPTIONS(MAKE_ATTR_SET_GET_BOOL);

#define ATTR_REGISTER(option, desc)                     \
        SIM_register_typed_attribute(                   \
                connection_class, #option,              \
                get_ ## option, NULL,                   \
                set_ ## option, NULL,                   \
                Sim_Attr_Optional | Sim_Attr_Read_Only, \
                "b", NULL,                              \
                "Enables " desc ". Default off.");

static attr_value_t
print_stats(conf_object_t *obj) {
        SIM_printf("Printing LIM stats\n");
        connection_t *con = obj_to_con(obj); 
        SIM_printf("Total address callback count:   %ld\n", (long int) con->total_addr_callback_cnt);
        SIM_printf("Encoded address callback count: %ld\n", (long int) con->encoded_addr_callback_cnt);
        float ratio = (100.0* (float)con->encoded_addr_callback_cnt) / (float)con->total_addr_callback_cnt;
        SIM_printf("Encoded to total ratio:         %.2f%%\n", ratio);
        SIM_printf("Metadata reads:                 %ld\n", (long int) con->metadata_reads_cnt);
        return SIM_make_attr_int64(0);
}
void
init_connection(void)
{
    static const class_data_t ic_funcs = {
            .alloc_object = ic_alloc,
            .description = "Instrumentation connection",
            .pre_delete_instance = ic_pre_delete_connection,
            .delete_instance = ic_delete_connection,
            .kind = Sim_Class_Kind_Session
    };
    connection_class = SIM_register_class("lim_model_connection",
                                            &ic_funcs);
    
    static const instrumentation_connection_interface_t ic_iface = {
            .enable = ic_enable,
            .disable = ic_disable,
    };
    SIM_REGISTER_INTERFACE(connection_class,
                            instrumentation_connection, &ic_iface);
    
 /* Register attributes */

        SIM_register_typed_attribute(
                connection_class, "cpu",
                get_cpu, NULL,
                NULL, NULL,
                Sim_Attr_Pseudo,
                "n|o", NULL,
                "to read out the cpu");

    SIM_register_attribute(
                connection_class, "print_stats",
                print_stats, NULL,
                Sim_Attr_Pseudo, "a", "Printing R/W stats");

    FOR_OPTIONS(ATTR_REGISTER);
}
