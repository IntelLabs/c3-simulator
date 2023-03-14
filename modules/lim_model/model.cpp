#include "model.h"
#ifndef DEV_DEBUG
#define DEV_DEBUG 0
#endif
#define CORRUPTED_ADDR 0x0

lim_class::lim_class(LimSimicsConnection *con) : con_(con) {}

bool lim_class::is_encoded_addr_debug_en() {
    // Update the constant to skip the specified number of encoded address debug
    // messages:
    return con_->debug_on && 0 < encoded_addr_callback_cnt_;
}

void lim_class::raise_page_fault_exception(logical_address_t la,
                                           access_t access_type) {
    if (curr_instr_is_prefetch()) {
        SIM_printf("Info: skipping spurious page fault from prefetch\n");
        return;
    }

    if (is_page_fault) {
        SIM_printf("WARNING: two consecutive page faults!\n");
        assert(0);
    }
    uint32_t ecode = (access_type & Sim_Access_Write) ? 0x6 : 0x4;

    // PF_fault does long jump, hence need to reset state
    is_encoded_pointer_ = false;

    is_page_fault = true;
    if (con_->debug_on)
        SIM_printf("\nPage fault la=0x%016llx, ecode=0x%x\n", la, ecode);
    con_->x86_ex_iface->PF_fault(con_->cpu_, la, ecode);
}
void lim_class::print_access_info(logical_address_t la) {
    SIM_printf("Count: %ld\n", (long int)encoded_addr_callback_cnt_);
    SIM_printf("LA: 0x%016llx (=> {P%x:%ld-byte}{T%x}0x%016llx)\n",
               (long long unsigned int)la, encoded_size,
               get_slot_size_in_bytes(encoded_size), ptr_tag,
               (long long unsigned int)la_decoded);
    SIM_printf("%d-byte metadata @0x%016llx:0x%016llxp: ", (int)meta_size,
               (long long unsigned int)la_meta,
               (long long unsigned int)pa_meta);
    SIM_printf("[0x%016llx, 0x%016llx]T%x",
               (long long unsigned int)dec_meta.lower_la,
               (long long unsigned int)dec_meta.upper_la, dec_meta.tag_left);
    if (meta_size != 1) {
        SIM_printf(":%x", dec_meta.tag_right);
    }
    SIM_printf("\n");
    if (page_crossing_type == Sim_Page_Crossing_None) {
        SIM_printf("page_crossing_type == Sim_Page_Crossing_None\n");
    } else if (page_crossing_type == Sim_Page_Crossing_First) {
        SIM_printf("page_crossing_type == Sim_Page_Crossing_First\n");
    } else {
        assert(0);
    }
}
FORCE_INLINE size_t calc_bytes_before_pg_bndry(uint64_t la, size_t size) {
    uint64_t distance_to_page_bndry =
            SIM_PAGE_SIZE - (la & LIM_FMASK(PAGE_OFFSET));
    assert(distance_to_page_bndry <= SIM_PAGE_SIZE);
    return (size_t)min_uint64(distance_to_page_bndry, size);
}
uint64_t lim_class::translate_la_to_pa(uint64_t la, access_t access_type) {
    physical_block_t pa_block;
    pa_block = con_->pi_iface->logical_to_physical(con_->cpu_, la, access_type);
    if (pa_block.valid == 0) {
        raise_page_fault_exception(la, access_type);
    } else {
        is_page_fault = false;
    }
    return pa_block.address;
}
void lim_class::read_from_linear_addr_samepage(logical_address_t la,
                                               void *rd_data, size_t size) {
    if (get_page_addr(la) != get_page_addr(la + size - 1)) {
        SIM_printf(
                "%s:%d WARNING: get_page_addr(la) !=get_page_addr(la+size-1)\n",
                __func__, __LINE__);
        SIM_printf("la = 0x%016llx\n", (long long unsigned int)la);
        SIM_printf("size=%ld\n", size);
        SIM_printf("encoded_addr_callback_cnt_=%lld\n",
                   encoded_addr_callback_cnt_);
    }
    uint64_t pa = translate_la_to_pa(la, Sim_Access_Read);
    if (pa == 0) {
        SIM_printf("Failed to translate LA %p for read.\n", (void *)la);
        assert(0);
    }
    int remaining_bytes = (int)size;
    uint64_t *buf = (uint64_t *)rd_data;
    while (remaining_bytes > 0) {
        int rd_len = min_int(remaining_bytes, 8);
        *buf = SIM_read_phys_memory(con_->cpu_, pa, rd_len);
        if (SIM_get_pending_exception() != SimExc_No_Exception) {
            SIM_printf("%s:%d: Exception while reading %d bytes of physical "
                       "memory @ %p (%s).\n",
                       __FILE__, __LINE__, rd_len, (void *)pa,
                       SIM_last_error());
            SIM_clear_exception();
            assert(0);
        }
        buf++;
        pa += 8;
        remaining_bytes -= 8;
    }
}
void lim_class::write_to_linear_addr_samepage(uint64_t la, const void *wr_data,
                                              size_t size) {
    if (get_page_addr(la) != get_page_addr(la + size - 1)) {
        SIM_printf(
                "%s:%d WARNING: get_page_addr(la) !=get_page_addr(la+size-1)\n",
                __func__, __LINE__);
        SIM_printf("la = 0x%016llx\n", (long long unsigned int)la);
        SIM_printf("size=%ld\n", size);
        SIM_printf("encoded_addr_callback_cnt_=%lld\n",
                   encoded_addr_callback_cnt_);
        assert(0);
    }
    uint64_t pa = translate_la_to_pa(la, Sim_Access_Write);
    if (pa == 0) {
        SIM_printf("Failed to translate LA %p for write.\n", (void *)la);
        assert(0);
    }
    int remaining_bytes = (int)size;
    uint64_t *buf = (uint64_t *)wr_data;
    while (remaining_bytes > 0) {
        int wr_len = min_int(remaining_bytes, 8);
        SIM_write_phys_memory(con_->cpu_, pa, *buf, wr_len);
        if (SIM_get_pending_exception() != SimExc_No_Exception) {
            SIM_printf("%s:%d: Exception while writing %d bytes of physical "
                       "memory @ %p (%s).\n",
                       __FILE__, __LINE__, wr_len, (void *)pa,
                       SIM_last_error());
            SIM_clear_exception();
            assert(0);
        }
        buf++;
        pa += 8;
        remaining_bytes -= 8;
    }
    // return SUCCESS;
}
void lim_class::write_to_linear_addr_crosspage(uint64_t la, const void *wr_data,
                                               size_t size) {
    assert(size <= 64);
    uint64_t bytes_before = calc_bytes_before_pg_bndry(la, size);
    assert(bytes_before <= size);
    int bytes_after = size - bytes_before;
    assert(bytes_after <= (int)size);
    assert(bytes_before + bytes_after == size);

    write_to_linear_addr_samepage(la, wr_data, bytes_before);
    if (bytes_after) {
        write_to_linear_addr_samepage(la + bytes_before,
                                      (uint8_t *)wr_data + bytes_before,
                                      bytes_after);
    }
}
void lim_class::read_from_linear_addr_crosspage(logical_address_t la,
                                                void *rd_data, size_t size) {
    assert(size <= 64);
    uint8_t buffer[64 + 8];
    uint64_t bytes_before = calc_bytes_before_pg_bndry(la, size);
    size_t bytes_after = size - bytes_before;
    assert(bytes_before <= size);
    assert(bytes_after <= size);
    assert(bytes_before + bytes_after == size);

    read_from_linear_addr_samepage(la, &buffer[0], bytes_before);
    if (bytes_after) {
        read_from_linear_addr_samepage(la + bytes_before, &buffer[bytes_before],
                                       bytes_after);
    }
    memcpy(rd_data, buffer, size);
}
bool lim_class::curr_instr_is_prefetch() {
    uint64 instr_la = con_->pi_iface->get_program_counter(con_->cpu_);
    uint16_t instr_buf;
    read_from_linear_addr_crosspage(instr_la, &instr_buf, sizeof(instr_buf));
    return instr_buf == 0x180f;  // Prefetch opcode
}
void lim_class::print_stats() {
    SIM_printf("Printing LIM stats\n");
    SIM_printf("Total write count:   %ld\n", (long int)total_write_cnt_);
    SIM_printf("Total read count:   %ld\n", (long int)total_read_cnt_);
    SIM_printf("Encoded write count:   %ld\n", (long int)encoded_write_cnt);
    SIM_printf("Encoded read count:   %ld\n", (long int)encoded_read_cnt);
    SIM_printf("Total address callback count:   %ld\n",
               (long int)total_addr_callback_cnt_);
    SIM_printf("Encoded address callback count: %ld\n",
               (long int)encoded_addr_callback_cnt_);
    float ratio = (100.0 * (float)encoded_addr_callback_cnt_) /
                  (float)total_addr_callback_cnt_;
    SIM_printf("Encoded to total ratio:         %.2f%%\n", ratio);
    SIM_printf("Metadata reads:                 %ld\n",
               (long int)metadata_reads_cnt);
}

logical_address_t lim_class::address_before(logical_address_t la,
                                            conf_object_t *cpu,
                                            address_handle_t *handle) {
    cpu_address_info_t *addr_info = (cpu_address_info_t *)handle;
    if (la != 0 && addr_info->size_ != 0) {
        total_addr_callback_cnt_++;
    }
    if (is_encoded_lim_ptr(la)) {
        ASSERT_MSG(con_->cpu_ == cpu, "Refactor fail");
        la_decoded = lim_decode_pointer(la);
        encoded_size = get_encoded_size(la);
        la_meta = get_metadata_address(la_decoded, encoded_size);
        meta_size = get_metadata_size(encoded_size);
        encoded_addr_callback_cnt_++;
        is_encoded_pointer_ = true;
        if (trace_only) {
            if (is_encoded_addr_debug_en()) {
                SIM_printf("******** LIM address detected. Count: %ld "
                           "*************\n",
                           (long int)encoded_addr_callback_cnt_);
                SIM_printf("  address_before : 0x%016llx\n",
                           (long long unsigned int)la);
                SIM_printf("  decoded address: 0x%016llx \n",
                           (long long unsigned int)la_decoded);
                SIM_printf("  la_meta:         0x%016llx \n",
                           (long long unsigned int)la_meta);
                SIM_printf("  meta_size:       %d\n", (int)meta_size);
            }
            return la_decoded;
        }
        uint64_t return_la = la_decoded;
        la_encoded = la;
        ptr_tag = get_encoded_tag(la);
        la_middle = get_middle_address(la_decoded, encoded_size);
        // Get metadata
        uint64_t metadata[LIM_METADATA_SIZE_512B / 8];
        uint64_t metadata_pa[LIM_METADATA_SIZE_512B / 8];
        physical_block_t pa_meta_block;
        for (size_t i = 0; i * 8 < meta_size; i++) {
            uint64_t la_to_translate = la_meta + i * 8;
            pa_meta_block = con_->pi_iface->logical_to_physical(
                    cpu, la_to_translate, Sim_Access_Read);
            if (pa_meta_block.valid == 0) {
                raise_page_fault_exception(la_to_translate, Sim_Access_Read);
            } else {
                is_page_fault = false;
                metadata_pa[i] = pa_meta_block.address;
                metadata[i] = SIM_read_phys_memory(cpu, metadata_pa[i], 8);
            }
        }

        pa_meta = metadata_pa[0];
        dec_meta = lim_decode_metadata(metadata, meta_size, la_middle);
        page_crossing_type =
                con_->x86_aq_iface->get_page_crossing_info(cpu, handle);
        if (is_encoded_addr_debug_en()) {
            SIM_printf("******** Tagged address detected. *************\n");
            print_access_info(la);
        }

        if (!con_->disable_meta_check) {
            // Check if tags match
            if (ptr_tag != dec_meta.tag_left ||
                dec_meta.tag_left != dec_meta.tag_right) {
                SIM_printf("LIM WARNING: tag mismatch! ptr_tag=%x "
                           "meta_tag=(%x,%x)\n",
                           ptr_tag, dec_meta.tag_left, dec_meta.tag_right);

                print_access_info(la);

                // GP_fault does long jump, hence need to reset state
                is_encoded_pointer_ = false;

                if (con_->debug_on)
                    SIM_printf("*** GP fault ***\n");
                con_->x86_ex_iface->GP_fault(cpu, 0, false, "Tag mismatch");

                // Unreachable
            }
            // Check if lower/upper la are within the slot
            if (dec_meta.lower_la < get_slot_start(la_decoded, encoded_size) ||
                dec_meta.upper_la > get_slot_end(la_decoded, encoded_size)) {
                SIM_printf("LIM WARNING: invalid boundary for encoded "
                           "la=0x%016llx\n",
                           (long long unsigned int)la);
                SIM_printf("Lower bound stored in metadata:              "
                           "0x%016llx\n",
                           (long long unsigned int)dec_meta.lower_la);
                SIM_printf("  Must be no lower than slot start:          "
                           "0x%016llx\n",
                           (long long unsigned int)get_slot_start(
                                   la_decoded, encoded_size));
                SIM_printf("Upper bound stored in metadata:              "
                           "0x%016llx\n",
                           (long long unsigned int)dec_meta.upper_la);
                SIM_printf("  Must be no greater than slot end:          "
                           "0x%016llx\n",
                           (long long unsigned int)get_slot_end(la_decoded,
                                                                encoded_size));
                SIM_printf("Intentionally corrupting pointer to 0x%016llx\n",
                           (long long unsigned int)CORRUPTED_ADDR);
                return_la = CORRUPTED_ADDR;
            }
        }

        if (con_->x86_aq_iface->get_page_crossing_info(cpu, handle) ==
            Sim_Page_Crossing_First) {
            if (con_->debug_on)
                SIM_printf("INFO: address_before: Sim_Page_Crossing_First\n");
            is_crossing_page_first = true;
        }

        // Check if the first byte is OOB and corrupt pointer.
        // The rest of the access will be checked in the subsequent callback for
        // the data access.
        if (!con_->disable_meta_check) {
            if (la_decoded < dec_meta.lower_la) {
                SIM_printf("LIM WARNING: OOB: crossing lower boundary for "
                           "la(decoded)=0x%016llx\n",
                           (long long unsigned int)la_decoded);
                SIM_printf("Allowed lower LA:                                  "
                           "       0x%016llx\n",
                           (long long unsigned int)dec_meta.lower_la);
                SIM_printf("Intentionally corrupting pointer to 0x%016llx\n",
                           (long long unsigned int)CORRUPTED_ADDR);
                return_la = CORRUPTED_ADDR;
            } else if (la_decoded > dec_meta.upper_la) {
                SIM_printf("LIM WARNING: OOB: crossing upper boundary for "
                           "la(decoded)=0x%016llx\n",
                           (long long unsigned int)la_decoded);
                SIM_printf("Allowed upper LA:                                  "
                           "       0x%016llx\n",
                           (long long unsigned int)dec_meta.upper_la -
                                   (con_->data_disp ? meta_size : 0));
                SIM_printf("Intentionally corrupting pointer to 0x%016llx\n",
                           (long long unsigned int)CORRUPTED_ADDR);
                return_la = CORRUPTED_ADDR;
            }
        }

        if (return_la == CORRUPTED_ADDR) {
            print_access_info(la);
            // GP_fault does long jump, hence need to reset state
            is_encoded_pointer_ = false;
            if (con_->debug_on)
                SIM_printf("*** GP fault ***\n");
            con_->x86_ex_iface->GP_fault(cpu, 0, false, "OOB");
        }

        return return_la;
    }
    if (is_encoded_pointer_ &&
        con_->x86_aq_iface->get_page_crossing_info(cpu, handle) ==
                Sim_Page_Crossing_Second) {
        // If here means that this is the second transaction from a cross-page
        // access using encoded pointer
        page_crossing_type = Sim_Page_Crossing_Second;
        la_decoded = lim_decode_pointer(la);
        if (con_->debug_on)
            SIM_printf("INFO: address_before: Sim_Page_Crossing_Second\n");
        if (con_->debug_on)
            SIM_printf("la :         0x%016llx\n", (long long unsigned int)la);
        if (con_->debug_on)
            SIM_printf("la_decoded : 0x%016llx\n",
                       (long long unsigned int)la_decoded);
    }
    if (is_encoded_pointer_) {
        if (con_->debug_on)
            SIM_printf("WARNING (addr callback) is_encoded_pointer_ set. la : "
                       "0x%016llx\n",
                       (long long unsigned int)la);
    }
    return la;
}
void lim_class::modify_data_on_mem_access(conf_object_t *obj,
                                          conf_object_t *cpu,
                                          memory_handle_t *mem,
                                          const char *rw) {
    ASSERT_MSG(con_->cpu_ == cpu, "Refactor fail");
    const cpu_memory_query_interface_t *mq = con_->mq_iface;
    uint64 la = mq->logical_address(cpu, mem);
    cpu_bytes_t bytes = mq->get_bytes(cpu, mem);
    unsigned size = mq->get_bytes(cpu, mem).size;

    // if trace only, just record the stats and return
    if (trace_only) {
        // SIM_printf("Trace_only OK");
        if (la == 0)
            return;
        if (rw[0] == 'R') {
            total_read_cnt_++;
            if (is_encoded_pointer_)
                encoded_read_cnt++;
        } else {
            total_write_cnt_++;
            if (is_encoded_pointer_)
                encoded_write_cnt++;
        }
        if (is_encoded_pointer_ &&
            mq->get_page_crossing_info(cpu, mem) != Sim_Page_Crossing_First) {
            is_encoded_pointer_ = false;
        }
        return;
    }

    // Continue if not tracing
    if (is_encoded_pointer_ && la == 0) {
        if (con_->debug_on)
            SIM_printf("(%d) WARNING (data callback) is_encoded_pointer_  with "
                       "la : 0x%016llx\n\n",
                       __LINE__, (long long unsigned int)la);
        return;
    }
    if (is_encoded_pointer_ && la != 0) {
        if (con_->debug_on)
            SIM_printf("data callback la=0x%016llx\n",
                       (long long unsigned int)la);
        if (la != la_decoded) {
            if (con_->debug_on)
                SIM_printf("(%d) WARNING (data callback) is_encoded_pointer_  "
                           "with la=0x%016llx != la_decoded=0x%016llx\n\n",
                           __LINE__, la, la_decoded);
            is_encoded_pointer_ = false;
            return;
        }

        // Check lower bound
        if (!con_->disable_meta_check) {
            if (la < dec_meta.lower_la) {
                SIM_printf("LIM WARNING: OOB: crossing lower boundary for "
                           "la=0x%016llx\n",
                           (long long unsigned int)la);
                SIM_printf("Allowed lower LA:                                "
                           "0x%016llx\n",
                           (long long unsigned int)dec_meta.lower_la);
                print_access_info(la);
                if (con_->break_on_decode_fault) {
                    SIM_printf("Breaking Simulation (line %d). Type run to "
                               "continue...\n",
                               __LINE__);
                    SIM_break_simulation("break_on_decode_fault");
                }
            }
        }

        uint64_t la_last_byte = la + size - 1;
        uint64_t la_last_byte_after_shift = la_last_byte + meta_size;

        // Checking upper bound
        if (!con_->disable_meta_check) {
            if (la_last_byte_after_shift > dec_meta.upper_la) {
                SIM_printf("LIM WARNING: OOB: crossing upper boundary for "
                           "la=0x%016llx\n",
                           (long long unsigned int)la);
                SIM_printf("Transaction size:                                  "
                           "%d\n",
                           (int)size);
                SIM_printf("Last byte LA:                                    "
                           "0x%016llx\n",
                           (long long unsigned int)la_last_byte);
                SIM_printf(
                        "Last byte LA after shift:                        "
                        "0x%016llx\n",
                        (long long unsigned int)(con_->data_disp
                                                         ? la_last_byte
                                                         : la_last_byte_after_shift));
                SIM_printf("Allowed upper LA:                                "
                           "0x%016llx\n",
                           (long long unsigned int)dec_meta.upper_la -
                                   (con_->data_disp ? meta_size : 0));
                print_access_info(la);
                if (con_->break_on_decode_fault)
                    SIM_printf("Breaking Simulation (line %d). Type run to "
                               "continue...\n",
                               __LINE__);
                if (con_->break_on_decode_fault) {
                    SIM_break_simulation("break_on_decode_fault");
                }

                // GP_fault does long jump, hence need to reset state
                is_encoded_pointer_ = false;

                if (con_->debug_on)
                    SIM_printf("*** GP fault ***\n");
                con_->x86_ex_iface->GP_fault(cpu, 0, false, "OOB");
            }
        }

        page_crossing_type = mq->get_page_crossing_info(
                cpu, mem);  // This updates page crossing info in case it was
                            // changed due to address shift
        // Trivial case: zero bytes. Just return
        if (bytes.size == 0) {
            is_encoded_pointer_ = false;
            // if (debug_on) SIM_printf("is_encoded_poiner = 0 (%d)\n",
            // __LINE__);
            return;
        }
        if (bytes.data == NULL) {
            // need to handle this case for trace-only. Return for now
            is_encoded_pointer_ = false;
            // if (debug_on) SIM_printf("is_encoded_poiner = 0 (%d)\n",
            // __LINE__);
            return;
        }
        if (con_->debug_on) {
            SIM_printf("bytes_orig (%s %dB) = ", rw, (int)bytes.size);
            for (int i = bytes.size - 1; i >= 0; i--) {
                SIM_printf("%02x ", bytes.data[i]);
            }
            SIM_printf("\n");
        }

        if (con_->data_disp) {
            if ((la_meta <= la_last_byte) && (la < la_meta + meta_size)) {
                uint64_t disp_base =
                        lim_compute_disp_base(la_meta, meta_size, &dec_meta);

                // Offset within access that first overlaps metadata:
                uint64_t disp_off = max_uint64(la, la_meta) - la;
                // First offset within metadata region that is overlapped by
                // requested access:
                uint64_t disp_acc_start = (la_meta < la) ? la - la_meta : 0;
                // Highest offset within metadata region that is overlapped by
                // requested access:
                uint64_t disp_acc_end =
                        min_uint64(la_last_byte, la_meta + meta_size - 1) -
                        la_meta;
                // Total amount of overlap with metadata region:
                uint64_t disp_amt = (disp_acc_end - disp_acc_start) + 1;

                if (is_encoded_addr_debug_en())
                    SIM_printf("Overlaps metadata; displacing @ offset %lx to "
                               "%016lx + [%lx, %lx] (%ld bytes)\n",
                               disp_off, disp_base, disp_acc_start,
                               disp_acc_end, disp_amt);

                uint8_t fixup_data[64 + LIM_METADATA_SIZE_512B];
                // Copy prefix (if any) preceding overlap with metadata:
                // read_from_linear_addr_crosspage(la, fixup_data, disp_off);
                memcpy(fixup_data, bytes.data, disp_off);
                uint64_t amt_so_far = disp_off;
                if (rw[0] == 'R') {
                    // Read from displaced data for portion of access
                    // overlapping metadata:
                    read_from_linear_addr_crosspage(disp_base + disp_acc_start,
                                                    fixup_data + amt_so_far,
                                                    disp_amt);
                } else {
                    // Read metadata into fixed up data so that it will be
                    // written back to the same location:
                    read_from_linear_addr_crosspage(
                            la + disp_off, fixup_data + amt_so_far, disp_amt);
                    // Write displaced data:
                    uint64_t write_start_addr = disp_base + disp_acc_start;
                    if (((la_meta <= write_start_addr) &&
                         (write_start_addr < la_meta + meta_size)) ||
                        ((write_start_addr < la_meta) &&
                         (la_meta < write_start_addr + disp_amt))) {
                        SIM_printf("Invalid LIM metadata configuration leading "
                                   "to displaced data overwriting metadata.");
                        print_access_info(la);
                        if (con_->break_on_decode_fault) {
                            SIM_break_simulation(
                                    "break_on_decode_fault: "
                                    "Stopping simulation due to invalid LIM "
                                    "metadata configuration.");
                        }
                        this->con_->gp_fault(0, false,
                                             "Bad LIM metadata configuration");
                    }
                    write_to_linear_addr_crosspage(
                            write_start_addr, bytes.data + disp_off, disp_amt);
                }
                amt_so_far += disp_amt;
                // Copy suffix (if any) following overlap with metadata:
                memcpy(fixup_data + amt_so_far, bytes.data + amt_so_far,
                       size - amt_so_far);

                cpu_bytes_t bytes_mod;
                bytes_mod.size = bytes.size;
                bytes_mod.data = (uint8_t *)fixup_data;
                mq->set_bytes(cpu, mem, bytes_mod);
                if (is_encoded_addr_debug_en()) {
                    SIM_printf("bytes_mod  (%s %dB) = ", rw,
                               (int)bytes_mod.size);
                    for (int i = bytes_mod.size - 1; i >= 0; i--) {
                        SIM_printf("%02x ", bytes_mod.data[i]);
                    }
                    SIM_printf("\n");
                }
            }
        } else {
            bool transactionIsBeforeMeta =
                    (la_last_byte < la_meta) ? true : false;
            bool transactionIsAfterMeta = (la >= la_meta) ? true : false;
            bool transactionIsOverMeta =
                    (!transactionIsBeforeMeta && !transactionIsAfterMeta);
            if (!transactionIsBeforeMeta) {
                if (get_page_addr(la) !=
                    get_page_addr(la_last_byte_after_shift)) {
                    // bytes shifted into the next page. Check if the
                    // translation for the next page is valid. If invalid, it
                    // will generate a page fault
                    translate_la_to_pa(
                            (la_last_byte_after_shift & 0xFFFFFFFFFFFFF000),
                            (rw[0] == 'W') ? Sim_Access_Write
                                           : Sim_Access_Read);
                } else {
                    is_page_fault = false;
                }
            }

            uint8_t fixup_data[64 + LIM_METADATA_SIZE_512B];
            if (transactionIsAfterMeta) {
                // Transaction after metadata
                if (rw[0] == 'R') {
                    read_from_linear_addr_crosspage(la + meta_size, fixup_data,
                                                    bytes.size);
                } else {
                    read_from_linear_addr_crosspage(
                            la, fixup_data, min_uint64(bytes.size, meta_size));
                    write_to_linear_addr_crosspage(la + meta_size, bytes.data,
                                                   bytes.size);
                    if (bytes.size > meta_size)
                        memcpy(&fixup_data[meta_size], bytes.data,
                               bytes.size - meta_size);
                }
                cpu_bytes_t bytes_mod;
                bytes_mod.size = bytes.size;
                bytes_mod.data = (uint8_t *)fixup_data;
                mq->set_bytes(cpu, mem, bytes_mod);
                if (is_encoded_addr_debug_en()) {
                    SIM_printf("bytes_mod  (%s %dB) = ", rw,
                               (int)bytes_mod.size);
                    for (int i = bytes_mod.size - 1; i >= 0; i--) {
                        SIM_printf("%02x ", bytes_mod.data[i]);
                    }
                    SIM_printf("\n");
                }
            }

            if (transactionIsOverMeta) {
                // Transaction over the metadata
                size_t bytes_before = la_meta - la;
                size_t bytes_after = bytes.size - bytes_before;
                assert(bytes_before != 0 && bytes_before < bytes.size);
                assert(bytes_after != 0 && bytes_after < bytes.size);
                if (rw[0] == 'R') {
                    read_from_linear_addr_samepage(la, &fixup_data[0],
                                                   bytes_before);
                    read_from_linear_addr_crosspage(la_meta + meta_size,
                                                    &fixup_data[bytes_before],
                                                    bytes_after);
                } else if (rw[0] == 'W') {
                    // Write
                    memcpy(fixup_data, bytes.data, bytes_before);
                    read_from_linear_addr_crosspage(
                            la_meta, &fixup_data[bytes_before], meta_size);
                    memcpy(&fixup_data[bytes_before + meta_size],
                           &bytes.data[bytes_before], bytes_after);
                    uint64_t la_after_meta = la_meta + meta_size;
                    write_to_linear_addr_crosspage(la_after_meta,
                                                   &bytes.data[bytes_before],
                                                   bytes_after);
                } else
                    assert(0);  // unknown rw[0] value
                cpu_bytes_t bytes_mod;
                bytes_mod.size = bytes.size;
                bytes_mod.data = (uint8_t *)fixup_data;
                mq->set_bytes(cpu, mem, bytes_mod);
            }
        }

        //           Resetting Flags
        if (page_crossing_type == Sim_Page_Crossing_None ||
            page_crossing_type == Sim_Page_Crossing_Second) {
            is_encoded_pointer_ = false;
            is_crossing_page_second_ = false;
        }

        if (page_crossing_type == Sim_Page_Crossing_First) {
            page_crossing_type = Sim_Page_Crossing_Second;
            is_crossing_page_first = false;
            is_crossing_page_second_ = true;
        }
        if (is_encoded_addr_debug_en())
            SIM_printf("\n");
    }
    return;
}

void lim_class::exception_before(conf_object_t *obj, conf_object_t *cpu,
                                 exception_handle_t *eq_handle,
                                 lang_void *unused) {
    auto *con = static_cast<LimSimicsConnection *>(SIM_object_data(obj));
    if (con->break_on_exception) {
        int exc_num = con->eq_iface->exception_number(cpu, eq_handle);
        if (exc_num < 32 && exc_num != 14) {
            SIM_printf("Breaking on exception #%d\n", exc_num);
            SIM_break_simulation("Break on exception");
        }
    }
}

void lim_class::register_callbacks(LimSimicsConnection *con) {
    con_->register_exception_before_cb(
            CPU_Exception_All,
            [](auto *obj, auto *cpu, auto *handle, auto *m) {
                static_cast<lim_class *>(m)->exception_before(obj, cpu, handle,
                                                              NULL);
            },
            static_cast<void *>(this));

    /* Register a callback that will be called for each instruction */
    con_->register_address_before_cb(
            [](auto *obj, auto *cpu, auto la, auto *handle, auto *m) {
                return static_cast<lim_class *>(m)->address_before(la, cpu,
                                                                   handle);
            },
            static_cast<void *>(this));

    con_->register_read_before_cb(
            CPU_Access_Scope_Explicit,
            [](auto *obj, auto *cpu, auto *mem, auto *m) {
                static_cast<lim_class *>(m)->modify_data_on_mem_access(
                        obj, cpu, mem, "Read ");
            },
            static_cast<void *>(this));
    con_->register_write_before_cb(
            CPU_Access_Scope_Explicit,
            [](auto *obj, auto *cpu, auto *mem, auto *m) {
                static_cast<lim_class *>(m)->modify_data_on_mem_access(
                        obj, cpu, mem, "Write ");
            },
            static_cast<void *>(this));
}

void lim_class::custom_model_init(LimSimicsConnection *con) {
    metadata_reads_cnt = 0;
    if (con_->trace_only) {
        SIM_printf(
                "[LIM] Disabling skip over metadata for trace-only config\n");
        trace_only = 1;
    } else {
        trace_only = 0;
    }
}
