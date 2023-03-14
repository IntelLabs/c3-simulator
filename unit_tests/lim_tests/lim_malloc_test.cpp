// module: lim
#include <math.h>
#include <string.h>
#include <cstdbool>
#include <cstdlib>  // for std::getenv
#include <gtest/gtest.h>

typedef uint64_t logical_address_t;
#ifndef TRACE_ONLY
int trace_only;
#define TRACE_ONLY
#endif
bool data_disp;
int lim_no_encode;
#include "lim_ptr_encoding.h"  // NOLINT
// Helper function to init ref data buffer with random data
#ifndef PAGE_SIZE
#define PAGE_SIZE SIM_PAGE_SIZE
#endif
#define SRAND_SEED 0
/*
static void init_reference_data_random(uint8_t * buffer, int num) {
    srand(SRAND_SEED);
    for (int i = 0; i<num; i++) {
        buffer[i] = (uint8_t) rand();
    }
}
*/
#ifndef MAGIC
#define MAGIC(n)                                                               \
    do {                                                                       \
        int simics_magic_instr_dummy;                                          \
        __asm__ __volatile__("cpuid"                                           \
                             : "=a"(simics_magic_instr_dummy)                  \
                             : "a"(0x4711 | ((unsigned)(n) << 16))             \
                             : "ecx", "edx", "ebx");                           \
    } while (0)
#endif

static void init_reference_data_sequential(uint8_t *buffer, int num) {
    uint8_t sequence = 0x0;
    for (int i = 0; i < num; i++) {
        buffer[i] = sequence++;
    }
}

template <typename _type> static inline _type min(_type a, _type b) {
    return (a < b) ? a : b;
}

/*
static void* align_ptr_on_next_page(void* ptr) {
    return (void*) (((uint64_t)ptr & 0xFFFFFFFFFFFFF000) + 0x1000);
}
*/
static void *align_ptr_on_next_4p_bndry(void *ptr) {
    return (void *)(((uint64_t)ptr &
                     (0xFFFFFFFFFFFFFFFF << (PAGE_OFFSET + 2))) +
                    PAGE_SIZE * 4);
}

static const size_t UNALIGNED_BUFFER_SIZE = 8 * PAGE_SIZE;
static uint8_t STATIC_BUFFER_8P_unaligned[UNALIGNED_BUFFER_SIZE];
static uint8_t *STATIC_BUFFER_4P_aligned =
        (uint8_t *)align_ptr_on_next_4p_bndry(STATIC_BUFFER_8P_unaligned);
uint8_t ref_data[4 * PAGE_SIZE];

const int MAX_ALLOCS = 100;
void *pointer_array[MAX_ALLOCS];

class RWTest : public ::testing::Test {
 protected:
    static const int max_size = 4 * PAGE_SIZE;

    void *encode(void *ptr, size_t data_size, lim_tag_t tag) {
        size_t total_alloc_size = get_min_metadata_size(data_size) + data_size;
        size_t meta_size = total_alloc_size - data_size;
        memset(ptr, 0x0, total_alloc_size);
        uint8_t encoded_size =
                calculate_encoded_size((uint64_t)ptr, total_alloc_size);
        // printf("Encoding pointer p=%p, alloc_size=%ld:  encoded_size=%x\n",
        // ptr, alloc_size, encoded_size);
        if (!trace_only)
            set_metadata(ptr, data_size, meta_size, tag);
        return lim_no_encode ? ptr
                             : (void *)lim_encode_pointer((uint64_t)ptr,
                                                          encoded_size, tag);
    }
    template <typename T> void write_ref_data(void *dest, size_t num_elements) {
        assert(num_elements * sizeof(T) <= max_size);
        T *ref_data_T = (T *)ref_data;
        T *dest_T = (T *)dest;
        for (size_t i = 0; i < num_elements; i++) {
            dest_T[i] = ref_data_T[i];
        }
    }
    template <typename T>
    bool read_data_using_enc_ptr(void *ptr_enc, size_t num_elements) {
        T *ref_data_T = (T *)ref_data;
        T *ptr_enc_T = (T *)ptr_enc;
        for (size_t i = 0; i < num_elements; i++) {
            if (ptr_enc_T[i] != ref_data_T[i]) {
                return false;
            }
        }
        return true;
    }
    void print_addr_around_ptr(uint8_t *ptr) {
        int left_offset = 4;
        int right_offset = 16;
        for (int i = -left_offset; i < right_offset; i++) {
            printf("%02x ", (uint8_t)((uint64_t)ptr + i));
        }
        printf("\n");
        printf("                      ^\n");
    }
    void print_memory_around_ptr(uint8_t *ptr) {
        int left_offset = 4;
        int right_offset = 16;
        for (int i = -left_offset; i < right_offset; i++) {
            printf("%02x ", *(ptr + i));
        }
        printf("\n");
    }
    ::testing::AssertionResult check_written_memory(void *ptr_enc,
                                                    size_t size) {
        uint8_t *ptr = (uint8_t *)lim_decode_pointer((uint64_t)ptr_enc);
        uint8_t encoded_size = get_encoded_size((uint64_t)ptr_enc);
        void *ptr_metadata = trace_only ? ptr
                                        : (void *)get_metadata_address(
                                                  (uint64_t)ptr, encoded_size);
        size_t left_bytes;
        if ((uint64_t)ptr_metadata > (uint64_t)ptr) {
            left_bytes = min(size, (uint64_t)ptr_metadata - (uint64_t)ptr);
        } else {
            left_bytes = 0;
        }
        size_t right_bytes = size - left_bytes;
        assert(size >= left_bytes);
        assert(size >= right_bytes);
        bool match_left =
                (memcmp(ptr, ref_data, left_bytes) == 0) ? true : false;
        if (!match_left) {
            for (size_t i = 0; i < left_bytes; i++) {
                if (ptr[i] != ref_data[i]) {
                    printf("ref_data: ");
                    print_memory_around_ptr(&ref_data[i]);
                    printf("actual  : ");
                    print_memory_around_ptr(&ptr[i]);
                    printf("addr    : ");
                    print_addr_around_ptr(&ptr[i]);
                    printf("Left mismatch at address %p\n", &ptr[i]);
                    printf("ptr[%ld]=0x%02x != 0x%02x (ref_data)\n", i, ptr[i],
                           ref_data[i]);
                    return ::testing::
                            AssertionFailure();  // << "left mismatch: ptr[" <<
                                                 // i << "]=" << ptr[i] << " !=
                                                 // ref_data=" << ref_data[i];
                }
            }
        }
        size_t meta_size = 0;
        lim_decoded_metadata_t md;
        if (!trace_only) {
            meta_size = get_metadata_size(encoded_size);
            uint64_t midpoint = get_middle_address((uint64_t)ptr, encoded_size);
            md = lim_decode_metadata(ptr_metadata, meta_size, midpoint);
        }
        bool match_right = true;
        for (size_t i = left_bytes; i < size; i++) {
            uint8_t chk_byte;
            if (data_disp) {
                uint8_t *cursor = ptr + i;
                if (!trace_only &&
                    cursor < (((uint8_t *)ptr_metadata) + meta_size)) {
                    uint64_t disp_base = lim_compute_disp_base(
                            (uintptr_t)ptr_metadata, meta_size, &md);
                    chk_byte =
                            ((uint8_t *)disp_base)[cursor -
                                                   ((uint8_t *)ptr_metadata)];
                } else {
                    chk_byte = *cursor;
                }
            } else {
                chk_byte = ptr[i + meta_size];
            }
            if (chk_byte != ref_data[i]) {
                printf("ref_data: ");
                print_memory_around_ptr(&ref_data[i]);
                printf("actual  : ");
                print_memory_around_ptr(&ptr[i]);
                printf("addr    : ");
                print_addr_around_ptr(&ptr[i]);
                printf("Right mismatch at address %p\n", &ptr[i]);
                printf("chk_byte@%ld=0x%02x != 0x%02x (ref_data)\n", i,
                       chk_byte, ref_data[i]);
                return ::testing::
                        AssertionFailure();  // << "right mismatch: ptr[" << i
                                             // << "+LIM_METADATA_SIZE]=" <<
                                             // ptr[i+LIM_METADATA_SIZE] << " !=
                                             // ref_data=" << ref_data[i];

                match_right = false;
            }
        }
        if (match_left && match_right) {
            return ::testing::AssertionSuccess();
        } else {
            return ::testing::AssertionFailure();
        }
    }
    void clear_buffer() {
        memset(STATIC_BUFFER_8P_unaligned, 0xff, UNALIGNED_BUFFER_SIZE);
    }
    void SetUp() override {
        // printf("ptr (aligned)=%p\n", ptr);
        init_reference_data_sequential(ref_data, max_size);
        clear_buffer();
    }
    void TearDown() override {}
};
/*
TEST(MallocT, SingleAlloc) {
    uint32_t exp_val = 0xFFFFFFFF;
    uint32_t * p = (uint32_t*) malloc(sizeof(uint32_t));
    *p = exp_val;
    ASSERT_EQ(exp_val, *p);
}
*/

TEST(PointerEncoding, StructSize) {
    encoded_pointer_t ep;
    lim_meta_1B_t meta_1B;
    lim_meta_2B_t meta_2B;
    lim_meta_8B_t meta_8B;
    lim_meta_16B_t meta_16B;
    ASSERT_EQ(sizeof(ep), 8);
    ASSERT_EQ(sizeof(meta_1B), 1);
    ASSERT_EQ(sizeof(meta_2B), 2);
    ASSERT_EQ(sizeof(meta_8B), 8);
    ASSERT_EQ(sizeof(meta_16B), 16);
}

TEST_F(RWTest, SingleWriteReadBeforeMetadata) {
    typedef uint32_t data_t;
    size_t total_size = 64;
    size_t size = total_size - LIM_METADATA_SIZE_32_64B;
    int offset = total_size / 2 - sizeof(data_t) -
                 LIM_METADATA_OFFSET_FROM_MIDDLE_32_64B;

    data_t *ptr_base = (data_t *)STATIC_BUFFER_4P_aligned;
    lim_tag_t tag = 0xF;
    data_t *ptr_base_enc = (data_t *)encode(ptr_base, size, tag);
    if (!lim_no_encode) {
        size_t encoded_size_in_bytes = get_slot_size_in_bytes(
                get_encoded_size((uint64_t)ptr_base_enc));
        ASSERT_EQ(encoded_size_in_bytes, total_size);
    }
    data_t *ptr_enc = (data_t *)((uint64_t)ptr_base_enc + offset);
    int num_elements = 1;
    write_ref_data<data_t>(ptr_enc, num_elements);
    ASSERT_TRUE(check_written_memory(ptr_enc, num_elements * sizeof(data_t)));
    ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));

    // size_t size=64-LIM_METADATA_SIZE;
    // typedef uint32_t data_t;
    // data_t* ptr = (data_t*) STATIC_BUFFER_4P_aligned;
    // data_t* ptr_enc = (data_t*) encode(ptr, size);
    // data_t exp_data = 0xabcd1234;
    // int index = 0;
    // ptr_enc[index] = exp_data;
    // // test if data was written correctly, skipping metadata
    // ASSERT_EQ(ptr[index], exp_data);
    // ASSERT_EQ(ptr_enc[index], exp_data);
    clear_buffer();
}
TEST_F(RWTest, SingleWriteReadAfterMeta) {
    typedef uint32_t data_t;
    size_t total_size = 64;
    size_t size = total_size - LIM_METADATA_SIZE_32_64B;
    int offset = total_size / 2;  //+ LIM_METADATA_SIZE_32_64B;

    data_t *ptr_base = (data_t *)STATIC_BUFFER_4P_aligned;
    lim_tag_t tag = 0xF;
    data_t *ptr_base_enc = (data_t *)encode(ptr_base, size, tag);
    if (!lim_no_encode) {
        size_t encoded_size_in_bytes = get_slot_size_in_bytes(
                get_encoded_size((uint64_t)ptr_base_enc));
        ASSERT_EQ(encoded_size_in_bytes, total_size);
    }
    data_t *ptr_enc = (data_t *)((uint64_t)ptr_base_enc + offset);
    int num_elements = 1;
    write_ref_data<data_t>(ptr_enc, num_elements);
    ASSERT_TRUE(check_written_memory(ptr_enc, num_elements * sizeof(data_t)));
    ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));

    clear_buffer();
}
TEST_F(RWTest, SingleWriteReadOverMetadata) {
    typedef uint32_t data_t;
    size_t total_size = 64;
    size_t size = total_size - LIM_METADATA_SIZE_32_64B;
    int offset = total_size / 2 - sizeof(data_t) / 2 -
                 LIM_METADATA_OFFSET_FROM_MIDDLE_32_64B;

    data_t *ptr_base = (data_t *)STATIC_BUFFER_4P_aligned;
    lim_tag_t tag = 0xF;
    data_t *ptr_base_enc = (data_t *)encode(ptr_base, size, tag);
    if (!lim_no_encode) {
        size_t encoded_size_in_bytes = get_slot_size_in_bytes(
                get_encoded_size((uint64_t)ptr_base_enc));
        ASSERT_EQ(encoded_size_in_bytes, total_size);
    }
    data_t *ptr_enc = (data_t *)((uint64_t)ptr_base_enc + offset);
    int num_elements = 1;
    write_ref_data<data_t>(ptr_enc, num_elements);
    ASSERT_TRUE(check_written_memory(ptr_enc, num_elements * sizeof(data_t)));
    ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));

    clear_buffer();
}
TEST_F(RWTest, SingleWriteReadBeforeMetadataCrossPage) {
    typedef uint32_t data_t;
    size_t total_size = 4 * PAGE_SIZE;
    size_t size = total_size - LIM_METADATA_SIZE_512B;
    int offset = PAGE_SIZE - sizeof(data_t) / 2;

    data_t *ptr_base = (data_t *)STATIC_BUFFER_4P_aligned;
    lim_tag_t tag = 0xF;
    data_t *ptr_base_enc = (data_t *)encode(ptr_base, size, tag);
    if (!lim_no_encode) {
        size_t encoded_size_in_bytes = get_slot_size_in_bytes(
                get_encoded_size((uint64_t)ptr_base_enc));
        ASSERT_EQ(encoded_size_in_bytes, total_size);
    }
    data_t *ptr_enc = (data_t *)((uint64_t)ptr_base_enc + offset);
    int num_elements = 1;
    write_ref_data<data_t>(ptr_enc, num_elements);
    ASSERT_TRUE(check_written_memory(ptr_enc, num_elements * sizeof(data_t)));
    ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));

    clear_buffer();
}
TEST_F(RWTest, SingleWriteReadAfterMeta_SingleCloseToPageBndry) {
    typedef uint32_t data_t;
    size_t total_size = 4 * PAGE_SIZE;
    size_t size = total_size - LIM_METADATA_SIZE_512B;
    int offset = 3 * PAGE_SIZE - sizeof(data_t) - LIM_METADATA_SIZE_512B;

    data_t *ptr_base = (data_t *)STATIC_BUFFER_4P_aligned;
    lim_tag_t tag = 0xF;
    data_t *ptr_base_enc = (data_t *)encode(ptr_base, size, tag);
    if (!lim_no_encode) {
        size_t encoded_size_in_bytes = get_slot_size_in_bytes(
                get_encoded_size((uint64_t)ptr_base_enc));
        ASSERT_EQ(encoded_size_in_bytes, total_size);
    }
    data_t *ptr_enc = (data_t *)((uint64_t)ptr_base_enc + offset);
    int num_elements = 1;
    write_ref_data<data_t>(ptr_enc, num_elements);
    ASSERT_TRUE(check_written_memory(ptr_enc, num_elements * sizeof(data_t)));
    ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));

    clear_buffer();
}
TEST_F(RWTest, SingleWriteReadAfterMeta_SingleToNextPage) {
    typedef uint32_t data_t;
    size_t total_size = 4 * PAGE_SIZE;
    size_t size = total_size - LIM_METADATA_SIZE_512B;
    int offset = 3 * PAGE_SIZE - sizeof(data_t);

    data_t *ptr_base = (data_t *)STATIC_BUFFER_4P_aligned;
    lim_tag_t tag = 0xF;
    data_t *ptr_base_enc = (data_t *)encode(ptr_base, size, tag);
    if (!lim_no_encode) {
        size_t encoded_size_in_bytes = get_slot_size_in_bytes(
                get_encoded_size((uint64_t)ptr_base_enc));
        ASSERT_EQ(encoded_size_in_bytes, total_size);
    }
    data_t *ptr_enc = (data_t *)((uint64_t)ptr_base_enc + offset);
    int num_elements = 1;
    write_ref_data<data_t>(ptr_enc, num_elements);
    ASSERT_TRUE(check_written_memory(ptr_enc, num_elements * sizeof(data_t)));
    ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));

    clear_buffer();
}
TEST_F(RWTest, SingleWriteReadAfterMeta_SingleToCross) {
    typedef uint32_t data_t;
    size_t total_size = 4 * PAGE_SIZE;
    size_t size = total_size - LIM_METADATA_SIZE_512B;
    int offset = 3 * PAGE_SIZE - sizeof(data_t) / 2 - LIM_METADATA_SIZE_512B;

    data_t *ptr_base = (data_t *)STATIC_BUFFER_4P_aligned;
    lim_tag_t tag = 0xF;
    data_t *ptr_base_enc = (data_t *)encode(ptr_base, size, tag);
    if (!lim_no_encode) {
        size_t encoded_size_in_bytes = get_slot_size_in_bytes(
                get_encoded_size((uint64_t)ptr_base_enc));
        ASSERT_EQ(encoded_size_in_bytes, total_size);
    }
    data_t *ptr_enc = (data_t *)((uint64_t)ptr_base_enc + offset);
    int num_elements = 1;
    write_ref_data<data_t>(ptr_enc, num_elements);
    // uint8_t* tmp = (uint8_t*) ((uint64_t) ptr_base + offset);
    // printf("Data at %p: ", tmp);
    // for (int i = 0; i<16; i++) {
    //     printf("%02x ", tmp[i]);
    // }
    // printf("\n");
    ASSERT_TRUE(check_written_memory(ptr_enc, num_elements * sizeof(data_t)));
    ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));

    clear_buffer();
}
TEST_F(RWTest, SingleWriteReadAfterMeta_CrossToCross) {
    typedef uint64_t data_t;
    size_t total_size = 4 * PAGE_SIZE;
    size_t size = total_size - LIM_METADATA_SIZE_512B;
    int offset = 3 * PAGE_SIZE - sizeof(data_t) / 4 - LIM_METADATA_SIZE_512B;

    data_t *ptr_base = (data_t *)STATIC_BUFFER_4P_aligned;
    lim_tag_t tag = 0xF;
    data_t *ptr_base_enc = (data_t *)encode(ptr_base, size, tag);
    if (!lim_no_encode) {
        size_t encoded_size_in_bytes = get_slot_size_in_bytes(
                get_encoded_size((uint64_t)ptr_base_enc));
        ASSERT_EQ(encoded_size_in_bytes, total_size);
    }
    data_t *ptr_enc = (data_t *)((uint64_t)ptr_base_enc + offset);
    int num_elements = 1;
    write_ref_data<data_t>(ptr_enc, num_elements);
    // uint8_t* tmp = (uint8_t*) ((uint64_t) ptr_base + offset);
    // printf("Data at %p: ", tmp);
    // for (int i = 0; i<16; i++) {
    //     printf("%02x ", tmp[i]);
    // }
    // printf("\n");
    ASSERT_TRUE(check_written_memory(ptr_enc, num_elements * sizeof(data_t)));
    ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));

    clear_buffer();
}
TEST_F(RWTest, SingleWriteReadAfterMeta_CrossToSingle) {
    typedef uint64_t data_t;
    size_t total_size = 4 * PAGE_SIZE;
    size_t size = total_size - LIM_METADATA_SIZE_512B;
    int offset = 3 * PAGE_SIZE - sizeof(data_t) / 2;

    data_t *ptr_base = (data_t *)STATIC_BUFFER_4P_aligned;
    lim_tag_t tag = 0xF;
    data_t *ptr_base_enc = (data_t *)encode(ptr_base, size, tag);
    if (!lim_no_encode) {
        size_t encoded_size_in_bytes = get_slot_size_in_bytes(
                get_encoded_size((uint64_t)ptr_base_enc));
        ASSERT_EQ(encoded_size_in_bytes, total_size);
    }
    data_t *ptr_enc = (data_t *)((uint64_t)ptr_base_enc + offset);
    int num_elements = 1;
    write_ref_data<data_t>(ptr_enc, num_elements);
    // uint8_t* tmp = (uint8_t*) ((uint64_t) ptr_base + offset);
    // printf("Data at %p: ", tmp);
    // for (int i = 0; i<16; i++) {
    //     printf("%02x ", tmp[i]);
    // }
    // printf("\n");
    ASSERT_TRUE(check_written_memory(ptr_enc, num_elements * sizeof(data_t)));
    ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));

    clear_buffer();
}
TEST_F(RWTest, SingleWriteReadOverMetadata_CrossPage) {
    typedef uint64_t data_t;
    size_t total_size = 4 * PAGE_SIZE;
    size_t size = total_size - LIM_METADATA_SIZE_512B;
    int offset = 2 * PAGE_SIZE - sizeof(data_t) / 2 -
                 LIM_METADATA_OFFSET_FROM_MIDDLE_512B;

    data_t *ptr_base = (data_t *)STATIC_BUFFER_4P_aligned;
    lim_tag_t tag = 0xF;
    data_t *ptr_base_enc = (data_t *)encode(ptr_base, size, tag);
    if (!lim_no_encode) {
        size_t encoded_size_in_bytes = get_slot_size_in_bytes(
                get_encoded_size((uint64_t)ptr_base_enc));
        ASSERT_EQ(encoded_size_in_bytes, total_size);
    }
    data_t *ptr_enc = (data_t *)((uint64_t)ptr_base_enc + offset);
    int num_elements = 1;
    write_ref_data<data_t>(ptr_enc, num_elements);
    // uint8_t* tmp = (uint8_t*) ((uint64_t) ptr_base + offset);
    // printf("Data at %p: ", tmp);
    // for (int i = 0; i<16; i++) {
    //     printf("%02x ", tmp[i]);
    // }
    // printf("\n");
    ASSERT_TRUE(check_written_memory(ptr_enc, num_elements * sizeof(data_t)));
    ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));

    clear_buffer();
}
TEST_F(RWTest, 1BWriteRead_4Page) {
    int limit = 4 * PAGE_SIZE;
    for (int size = 8; size + LIM_METADATA_SIZE_512B <= limit;
         size = size + 8) {
        typedef uint8_t data_t;
        data_t *ptr = (data_t *)STATIC_BUFFER_4P_aligned;
        lim_tag_t tag = 0xF;
        data_t *ptr_enc = (data_t *)encode(ptr, size, tag);
        size_t num_elements = size / sizeof(data_t);
        write_ref_data<data_t>(ptr_enc, num_elements);
        // test if data was written correctly, skipping metadata
        ASSERT_TRUE(
                check_written_memory(ptr_enc, num_elements * sizeof(data_t)));
        ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));
        clear_buffer();
    }
}

TEST_F(RWTest, 2BWriteRead_4Page) {
    int limit = 4 * PAGE_SIZE;
    for (int size = 8; size + LIM_METADATA_SIZE_512B <= limit;
         size = size + 8) {
        typedef uint16_t data_t;
        for (uint64_t offset = 0; offset < sizeof(data_t); offset++) {
            data_t *ptr = (data_t *)STATIC_BUFFER_4P_aligned;
            lim_tag_t tag = 0xF;
            data_t *ptr_enc = (data_t *)encode(ptr, size, tag);

            ptr_enc = (data_t *)((uint64_t)ptr_enc + offset);
            size_t num_elements = (size - offset) / sizeof(data_t);
            // printf("size=%d ptr=%p ptr_enc=%p num_elements=%ld\n", size, ptr,
            // ptr_enc, num_elements);
            write_ref_data<data_t>(ptr_enc, num_elements);
            // test if data was written correctly, skipping metadata
            ASSERT_TRUE(check_written_memory(ptr_enc,
                                             num_elements * sizeof(data_t)));
            ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));
            clear_buffer();
        }
    }
}
TEST_F(RWTest, 4BWriteRead_4Page) {
    int limit = 4 * PAGE_SIZE;
    for (int size = 8; size + LIM_METADATA_SIZE_512B <= limit;
         size = size + 8) {
        typedef uint32_t data_t;
        for (uint64_t offset = 0; offset < sizeof(data_t); offset++) {
            data_t *ptr = (data_t *)STATIC_BUFFER_4P_aligned;
            lim_tag_t tag = 0xF;
            data_t *ptr_enc = (data_t *)encode(ptr, size, tag);

            ptr_enc = (data_t *)((uint64_t)ptr_enc + offset);
            size_t num_elements = (size - offset) / sizeof(data_t);
            // printf("size=%d ptr=%p ptr_enc=%p num_elements=%ld\n", size, ptr,
            // ptr_enc, num_elements);
            write_ref_data<data_t>(ptr_enc, num_elements);
            // test if data was written correctly, skipping metadata
            ASSERT_TRUE(check_written_memory(ptr_enc,
                                             num_elements * sizeof(data_t)));
            ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));
            clear_buffer();
        }
    }
}
TEST_F(RWTest, 8BWriteRead_4Page) {
    int limit = 4 * PAGE_SIZE;
    for (int size = 8; size + LIM_METADATA_SIZE_512B <= limit;
         size = size + 8) {
        typedef uint64_t data_t;
        for (uint64_t offset = 0; offset < sizeof(data_t); offset++) {
            data_t *ptr = (data_t *)STATIC_BUFFER_4P_aligned;
            lim_tag_t tag = 0xF;
            data_t *ptr_enc = (data_t *)encode(ptr, size, tag);
            ptr_enc = (data_t *)((uint64_t)ptr_enc + offset);
            size_t num_elements = (size - offset) / sizeof(data_t);
            // printf("size=%d ptr=%p ptr_enc=%p num_elements=%ld\n", size, ptr,
            // ptr_enc, num_elements);
            write_ref_data<data_t>(ptr_enc, num_elements);
            // test if data was written correctly, skipping metadata
            ASSERT_TRUE(check_written_memory(ptr_enc,
                                             num_elements * sizeof(data_t)));
            ASSERT_TRUE(read_data_using_enc_ptr<data_t>(ptr_enc, num_elements));
            clear_buffer();
        }
    }
}

TEST(Malloc, ManySmallAllocsThenFree) {
    srand(0);
    size_t max_alloc_size = SIM_CACHELINE_SIZE;
    for (int i = 0; i < MAX_ALLOCS; i++) {
        size_t size = (size_t)rand() % max_alloc_size;
        pointer_array[i] = malloc(size);
        bool pointer_is_null = (pointer_array[i] == NULL) ? true : false;
        ASSERT_FALSE(pointer_is_null);
    }
    for (int i = 0; i < MAX_ALLOCS; i++) {
        free(pointer_array[i]);
    }
}

TEST(Malloc, AllocThen_8B_to_64B_Reads) {
    int max_size = 1000;
    srand(0);
    uint8_t exp_val8[max_size];  // NOLINT
    uint8_t *p8 = (uint8_t *)malloc(max_size);
    // for (int offset = 0; offset < 64; offset+)
    for (int i = 0; i < max_size; i++) {
        exp_val8[i] = (uint8_t)rand();
        p8[i] = exp_val8[i];
    }

    for (int i8 = 0; i8 < max_size; i8++) {
        ASSERT_EQ(exp_val8[i8], p8[i8]);
    }
    uint16_t *exp_val16 = (uint16_t *)exp_val8;
    uint16_t *p16 = (uint16_t *)p8;
    for (int i16 = 0; i16 < max_size / 2; i16++) {
        ASSERT_EQ(exp_val16[i16], p16[i16]);
    }
    uint32_t *exp_val32 = (uint32_t *)exp_val8;
    uint32_t *p32 = (uint32_t *)p8;
    for (int i32 = 0; i32 < max_size / 4; i32++) {
        ASSERT_EQ(exp_val32[i32], p32[i32]);
    }
    uint64_t *exp_val64 = (uint64_t *)exp_val8;
    uint64_t *p64 = (uint64_t *)p8;
    for (int i64 = 0; i64 < max_size / 8; i64++) {
        ASSERT_EQ(exp_val64[i64], p64[i64]);
    }
    free(p8);
}
TEST(Malloc, AllocThen_8B_to_64B_ReadsWithOffset) {
    int max_size = 10000;
    srand(0);
    uint8_t exp_val8[max_size];  // NOLINT
    uint8_t *p8 = (uint8_t *)calloc(1, max_size);
    for (int i = 0; i < max_size; i++) {
        exp_val8[i] = (uint8_t)i;  // rand() ;
        p8[i] = exp_val8[i];
    }
    for (int offset = 1; offset < 64; offset++) {
        uint16_t *exp_val16 = (uint16_t *)(exp_val8 + offset);
        uint16_t *p16 = (uint16_t *)(p8 + offset);
        for (int i16 = 0; i16 < (max_size - offset) / 2; i16++) {
            ASSERT_EQ(exp_val16[i16], p16[i16]);
        }
        uint32_t *exp_val32 = (uint32_t *)(exp_val8 + offset);
        uint32_t *p32 = (uint32_t *)(p8 + offset);
        for (int i32 = 0; i32 < (max_size - offset) / 4; i32++) {
            ASSERT_EQ(exp_val32[i32], p32[i32]);
        }
        uint64_t *exp_val64 = (uint64_t *)(exp_val8 + offset);
        uint64_t *p64 = (uint64_t *)(p8 + offset);
        for (int i64 = 0; i64 < (max_size - offset) / 8; i64++) {
            ASSERT_EQ(exp_val64[i64], p64[i64]);
        }
    }
    free(p8);
}
TEST(Malloc, CheckOldData_LargeAlloc) {
    srand(123456);
    size_t max_size = 10000;
    uint8_t ref_data[max_size];  // NOLINT
    for (unsigned int iteration = 0; iteration < max_size; iteration++) {
        size_t size = (size_t)iteration;
        // printf("************ iteration=%d ************\n", iteration);
        for (unsigned int i = 0; i < min<size_t>(size, max_size); i++) {
            ref_data[i] = (uint8_t)rand();
        }
        uint8_t *p = (uint8_t *)malloc(size);
        for (unsigned int offset = 0; offset < min<size_t>(size, max_size);
             offset++) {
            p[offset] = ref_data[offset];
        }
        // printf("GTEST: size = %d bytes, new_size = %d bytes\n", (int) size,
        // (int) new_size);
        if (p == NULL) {
            printf("GTEST: p_new is NULL\n");
            printf("GTEST: size    =%d\n", (int)size);
            ASSERT_TRUE(0);
        }
        for (unsigned int offset = 0; offset < size; offset++) {
            int match = (ref_data[offset] == p[offset]) ? 1 : 0;
            if (!match) {
                printf("iteration=%d\n", iteration);
                printf("Mismatch on malloc:\n");
                printf("size    =%d\n", (int)size);
                printf("offset  =%d\n", (int)offset);
                printf("ref_data  =0x%02x\n", ref_data[offset]);
                printf("p         =0x%02x\n", p[offset]);
            }
            ASSERT_TRUE(match);
        }
        free((void *)p);
    }
}

TEST(Calloc, SingleAlloc) {
    uint32_t exp_val = 0x0;
    uint32_t *p = (uint32_t *)calloc(sizeof(uint32_t), 1);
    ASSERT_EQ(exp_val, *p);
    free(p);
}
TEST(Calloc, AllocRanging1Bto128M) {
    int max_size = 1 < 20;
    int max_num = 128;
    for (int size = 1; size <= max_size * max_num; size = size * max_num) {
        for (int num = 1; num <= max_num; num++) {
            // printf("CALLOC: num=%d, size=%d, total=%d\n", num, size,
            // num*size);
            uint8_t *p = (uint8_t *)calloc(num, size);
            // check for zero
            for (int ind = 0; ind < num * size; ind++) {
                ASSERT_EQ(0x0, p[ind]);
            }
            free(p);
        }
    }
}
TEST(Realloc, SmallToLarge) {
    // pointer_key_t crypto_key;
    // init_crypto_key_struct(&crypto_key);
    size_t small_size = 8;
    size_t large_size = 1 << 30;
    // init reference data
    size_t ref_size = small_size;
    uint8_t ref_data[ref_size];  // NOLINT
    for (size_t i = 0; i < ref_size; i++) {
        ref_data[i] = (uint8_t)rand();
    }
    uint8_t *p = (uint8_t *)malloc(small_size);
    // init p with reference data
    for (size_t i = 0; i < ref_size; i++) {
        p[i] = ref_data[i];
    }
    uint8_t *p_new = (uint8_t *)realloc(p, large_size);
    // check whether old data survived
    for (size_t i = 0; i < ref_size; i++) {
        ASSERT_EQ(ref_data[i], p_new[i]) << " i=" << i << std::endl;
    }
    free(p_new);
}
TEST(Realloc, LargeToSmall) {
    // pointer_key_t crypto_key;
    // init_crypto_key_struct(&crypto_key);
    int small_size = 8;
    int large_size = 1 << 30;
    // init reference data
    int ref_size = small_size;
    uint8_t ref_data[ref_size];  // NOLINT
    for (int i = 0; i < ref_size; i++) {
        ref_data[i] = (uint8_t)rand();
    }
    uint8_t *p = (uint8_t *)malloc(large_size);
    // ASSERT_TRUE(is_canonical((uint64_t) p));
    // init p with reference data
    for (int i = 0; i < ref_size; i++) {
        p[i] = ref_data[i];
    }
    uint8_t *p_new = (uint8_t *)realloc(p, small_size);
    // ASSERT_FALSE(is_canonical((uint64_t) p_new));
    // check whether old data survived
    for (int i = 0; i < ref_size; i++) {
        ASSERT_EQ(ref_data[i], p_new[i]);
    }
    free(p_new);
}
TEST(Realloc, CheckOldData_LargeAlloc) {
    // pointer_key_t crypto_key;
    // init_crypto_key_struct(&crypto_key);
    srand(1234);
    size_t max_size = 10000;
    uint8_t ref_data[max_size];  // NOLINT
    for (unsigned int i = 0; i < max_size; i++) {
        size_t size = 1 + (((size_t)rand()) % max_size);
        // printf("Count=%ld size=%ld\n", ++count, size);
        for (unsigned int i = 0; i < size; i++) {
            ref_data[i] = (uint8_t)rand();
        }
        uint8_t *p = (uint8_t *)malloc(size);
        for (unsigned int offset = 0; offset < size; offset++) {
            p[offset] = ref_data[offset];
        }

        // Parse the original metadata now in case it's needed for debug output
        // below before the metadata gets overwritten during realloc:
        uint64_t p_md_la;
        size_t p_md_sz;
        lim_decoded_metadata_t p_md;
        if (!trace_only) {
            uint64_t p_dec = lim_decode_pointer((uintptr_t)p);
            p_md_la = get_metadata_address_from_enc_ptr((uintptr_t)p);
            uint8_t p_enc_sz = get_encoded_size((uintptr_t)p);
            p_md_sz = get_metadata_size(p_enc_sz);
            uint64_t p_mid_la = get_middle_address(p_dec, p_enc_sz);
            p_md = lim_decode_metadata((void *)p_md_la, p_md_sz, p_mid_la);
        }

        int new_size = 1 + (((int)rand()) % max_size);
        // printf ("GTEST: new_size=%d\n", (int) new_size);
        // printf("************ iteration i=%d ************\n", i);
        // printf("GTEST: size = %d bytes, new_size = %d bytes\n", (int) size,
        // (int) new_size);
        uint8_t *p_new = (uint8_t *)realloc((void *)p, new_size);
        if (p_new == NULL) {
            printf("GTEST: p_new is NULL\n");
            printf("GTEST: size    =%d\n", (int)size);
            printf("GTEST: new_size=%d\n", (int)new_size);
            ASSERT_TRUE(0);
        }
        for (unsigned int offset = 0; offset < min<size_t>(size, new_size);
             offset++) {
            int match = (ref_data[offset] == p_new[offset]) ? 1 : 0;
            if (!match) {
                lim_decoded_metadata_t p_new_md;
                uint64_t p_new_md_la;
                size_t p_new_md_sz;
                if (trace_only) {
                    printf("*** Ignore metadata-derived values below, because "
                           "no metadata is set in trace-only mode. ***\n");
                } else {
                    uint64_t p_new_dec = lim_decode_pointer((uintptr_t)p_new);
                    p_new_md_la =
                            get_metadata_address_from_enc_ptr((uintptr_t)p_new);
                    uint8_t p_new_enc_sz = get_encoded_size((uintptr_t)p_new);
                    p_new_md_sz = get_metadata_size(p_new_enc_sz);
                    uint64_t p_new_mid_la =
                            get_middle_address(p_new_dec, p_new_enc_sz);
                    p_new_md = lim_decode_metadata((void *)p_new_md_la,
                                                   p_new_md_sz, p_new_mid_la);
                }
                printf("i=%d\n", i);
                printf("Mismatch on realloc:\n");
                printf("size    =%d\n", (int)size);
                printf("new_size=%d\n", (int)new_size);
                printf("p       =0x%016llx [%016llx, %016llx] (p_meta: "
                       "0x%016llx w/ size %ld)\n",
                       p, p_md.lower_la, p_md.upper_la, p_md_la, p_md_sz);
                printf("p_new   =0x%016llx [%016llx, %016llx] (p_meta: "
                       "0x%016llx w/ size %ld)\n",
                       p_new, p_new_md.lower_la, p_new_md.upper_la, p_new_md_la,
                       p_new_md_sz);
                printf("offset  =%d (p+off: 0x%016llx) (p_new+off: "
                       "0x%016llx)\n",
                       (int)offset, p + offset, p_new + offset);
                printf("ref_byte=0x%02x\n", ref_data[offset]);
                printf("chk_byte=0x%02x\n", p_new[offset]);
            }
            ASSERT_TRUE(match);
        }
        // printf("freeing p_new=%p\n", p_new);
        free((void *)p_new);
    }
}

int main(int argc, char **argv) {
    data_disp = (getenv("LIM_DATA_DISP") != NULL);
    lim_no_encode = (getenv("LIM_NO_ENCODE") != NULL);
    trace_only = lim_no_encode || (getenv("TRACE_ONLY") != NULL);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
