#include <gtest/gtest.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <assert.h>
#include <stdarg.h> // for va_list, va_start, va_end

#define MAGIC(n) do {                                                       \
  int simics_magic_instr_dummy;                                       \
  __asm__ __volatile__ ("cpuid"                                       \
	: "=a" (simics_magic_instr_dummy)             \
	: "a" (0x4711 | ((unsigned)(n) << 16))        \
	: "ecx", "edx", "ebx");                       \
} while (0)

#define SRAND_SEED 0

// Helper function to init ref data buffer with random data
/*
static void init_reference_data_random(uint8_t * buffer, int num) {
    srand(SRAND_SEED);
    for (int i = 0; i<num; i++) {
        buffer[i] = (uint8_t) rand();
    }
}
*/

static void init_reference_data_sequential(uint8_t * buffer, int num) {
    uint8_t sequence=0x0;
    for (int i = 0; i<num; i++) {
        buffer[i] = sequence++;
    }
}

/*
static void init_reference_data_0x11(uint8_t * buffer, int num) {
    for (int i = 0; i<num; i++) {
        buffer[i] = 0x11;
    }
}
*/
/*
TEST(MEMCPY, Case4849) {
    int max_size = 1 << 20;
    int ref_size = max_size;
	uint8_t ref_data[ref_size];
    init_reference_data_0x11(ref_data, ref_size);
    int bytes = 4849;
    uint8_t* p = (uint8_t*) calloc (1, bytes);
    memcpy((void*) p, (void*) ref_data, bytes);
    for (int i = 0; i < bytes; i++) {
        if (ref_data[i] != p[i]){
            MAGIC(0);
            printf("ERROR: Mismatch detected:\n");
            printf("num bytes=%d, i=%d, p=0x%016lx\n", bytes, i, (uint64_t) p);
            ASSERT_EQ(ref_data[i], p[i]);
        }
    }
    free(p);
}
*/

size_t BLI_snprintf(char * dst, size_t maxncpy, const char * format, ...) {
	va_list arg;
	va_start(arg, format);
	size_t n = vsnprintf(dst, maxncpy, format, arg);
	va_end(arg);
	return n;
}
void fill_string(char* str, size_t size) {
    if (size !=0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = '!' + i % ('~'-'!'); // pick any ascii between ! and ~
        }
    }
    str[size] = '\0';
}

TEST(STRING, strlen) {
    const char str[] = "abcdef0123";
    int length_expected = 10;
    int length = strlen(str);
    ASSERT_EQ(length, length_expected);
}

TEST(STRING, strnlen1) {
    const char str[] = "abcdef0123";
    int length_expected = 5;
    int length = strnlen(str, 5);
    ASSERT_EQ(length, length_expected);
}

TEST(STRING, wcslen) {
    const wchar_t str[] = L"abcdef0123";
    int length_expected = 10;
    int length = wcslen(str);
    ASSERT_EQ(length, length_expected);
}

TEST(STRING, MallocStrlen) {
    srand(SRAND_SEED);
    char* str;
    size_t max_length = 100;
    for (size_t alloc_length = 1; alloc_length <= max_length; alloc_length++) {
        str = (char*) calloc(1, alloc_length);
        for (size_t length_expected = 0; length_expected < alloc_length; length_expected++) {
            fill_string(str, length_expected);
            size_t length = strlen(str);
            if (length != length_expected) {
                printf("alloc_length    = %ld\n", alloc_length);
                printf("length_expected = %ld\n", length_expected);
            }
            ASSERT_EQ(length, length_expected);
        }
        free (str);
    }
}
TEST(FORMAT, blender_vsnprintf) {
    const char* path = "cube_####";
    const char* out_str_expected = "cube_0001";
    int ch_sta = 5;
    int ch_end = 9;
    int frame = 1;
    char out_str[1024];
    BLI_snprintf(out_str, sizeof(out_str),
                        "%.*s%.*d%s",
		                ch_sta, path, ch_end - ch_sta, frame, path + ch_end);
    ASSERT_STREQ(out_str, out_str_expected);
}

TEST(MEMCPY, CopyNBytesFromRefToBuffer) {
    int max_size = 1 << 20;
    int ref_size = max_size;
	uint8_t ref_data[ref_size];
    init_reference_data_sequential(ref_data, ref_size);
    for (int bytes = 0; bytes < (1<<13); bytes++) {
        uint8_t* p = (uint8_t*) calloc (1, bytes);
        /*
        if (bytes==4856){
            printf("num bytes=%d, p=0x%016lx\n", bytes, (uint64_t) p);
            printf("&p[4840] = 0x%016lx\n",  (uint64_t) &p[4840]);
            printf("MAGIC BREAKPOINT before memcpy\n");
            MAGIC(0);
            memcpy((void*) p, (void*) ref_data, bytes);
            printf("MAGIC BREAKPOINT after memcpy\n");
            MAGIC(0);
        } else {
            memcpy((void*) p, (void*) ref_data, bytes);
        } 
        */
        memcpy((void*) p, (void*) ref_data, bytes);
        for (int i = 0; i < bytes; i++) {
            if (ref_data[i] != p[i]){
                printf("ERROR: Mismatch detected:\n");
                printf("num bytes=%d, i=%d, p=0x%016lx\n", bytes, i, (uint64_t) p);
                printf("MAGIC BREAKPOINT after mismatch\n");
                MAGIC(0);
                ASSERT_EQ(ref_data[i], p[i]);
            }
        }
        free(p);
    }
}   

/*
TEST(WCSTOMBS, GetLength){
    char str[] = "t5.xml";
    wchar_t * w_buf = (wchar_t*) malloc(32);
    int length = strlen(str);
    for (int i = 0; i < length; i++) {
        w_buf[i] = (wchar_t) str[i];
    }
    char* buf = new char[length+1];
    int num = wcstombs(buf, w_buf, length);
    ASSERT_EQ(num, 6);
    ASSERT_EQ(strcmp(buf, str), 0);
}
*/

int main (int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
