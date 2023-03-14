// model: cc
// #define DEBUG
#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

TEST(VERSION, ManualVersionEncoding) {
    if (!have_cc_heap_encoding()) {
        fprintf(stderr, "no heap encoding, skipping\n");
        return;
    }

    uint64_t ptr = 0;
    uint64_t version = 0;

    ptr_metadata_t md = {.size_ = 8};

    md.version_ = version++ % VERSION_SIZE;
    auto ptr1 = cc_isa_encptr(ptr, &md);

    md.version_ = version++ % VERSION_SIZE;
    auto ptr2 = cc_isa_encptr(ptr, &md);

    md.version_ = version++ % VERSION_SIZE;
    auto ptr3 = cc_isa_encptr(ptr, &md);

    ASSERT_NE(ptr, ptr1);
    ASSERT_NE(ptr, ptr2);
    ASSERT_NE(ptr, ptr3);
    ASSERT_NE(ptr1, ptr2);
    ASSERT_NE(ptr1, ptr3);
    ASSERT_NE(ptr2, ptr3);

    dbgprint("%-10s %016lx", "ptr", ptr);
    dbgprint("%-10s %016lx %016lx", "ptr1", ptr1, cc_isa_decptr(ptr1));
    dbgprint("%-10s %016lx %016lx", "ptr2", ptr2, cc_isa_decptr(ptr2));
    dbgprint("%-10s %016lx %016lx", "ptr3", ptr3, cc_isa_decptr(ptr3));

    ASSERT_EQ(ptr, cc_isa_decptr(ptr1));
    ASSERT_EQ(ptr, cc_isa_decptr(ptr2));
    ASSERT_EQ(ptr, cc_isa_decptr(ptr3));
}

TEST(VERSION, ManualVersionEncodingWithMalloc) {
    if (!have_cc_heap_encoding()) {
        fprintf(stderr, "no heap encoding, skipping\n");
        return;
    }

    uint64_t ptr = (uint64_t)malloc(128);
    strncpy((char *)ptr, "Hello World\n", sizeof("Hello World\n"));
    uint64_t version = 0;

    uint64_t la = cc_isa_decptr((uint64_t)ptr);
    ptr_metadata_t md = {.size_ = 8};

    md.version_ = version++ % VERSION_SIZE;
    auto ptr1 = cc_isa_encptr(la, &md);

    md.version_ = version++ % VERSION_SIZE;
    auto ptr2 = cc_isa_encptr(la, &md);

    md.version_ = version++ % VERSION_SIZE;
    auto ptr3 = cc_isa_encptr(la, &md);

    ASSERT_NE(ptr, ptr1);
    ASSERT_NE(ptr, ptr2);
    ASSERT_NE(ptr, ptr3);
    ASSERT_NE(ptr1, ptr2);
    ASSERT_NE(ptr1, ptr3);
    ASSERT_NE(ptr2, ptr3);

    dbgprint("%-10s %016lx", "ptr", ptr);
    dbgprint("%-10s %016lx %016lx", "ptr1", ptr1, cc_isa_decptr(ptr1));
    dbgprint("%-10s %016lx %016lx", "ptr2", ptr2, cc_isa_decptr(ptr2));
    dbgprint("%-10s %016lx %016lx", "ptr3", ptr3, cc_isa_decptr(ptr3));

    ASSERT_EQ(la, cc_isa_decptr(ptr1));
    ASSERT_EQ(la, cc_isa_decptr(ptr2));
    ASSERT_EQ(la, cc_isa_decptr(ptr3));
}

TEST(VERSION, WithDataAccesses) {
    if (!have_cc_heap_encoding()) {
        fprintf(stderr, "no heap encoding, skipping\n");
        return;
    }

    char *ptr = (char *)malloc(128);
    char *ptr1_view = (char *)malloc(128);
    char *ptr2_view = (char *)malloc(128);
    char *ptr3_view = (char *)malloc(128);
    strncpy(ptr, "Hello World\n", sizeof("Hello World\n"));
    dbgprint("%-10s %s", "ptr", ptr);

    uint64_t version = 0;
    ptr_metadata_t md = {.size_ = 32};
    uint64_t la = cc_isa_decptr((uint64_t)ptr);

    md.version_ = version++ % VERSION_SIZE;
    auto ptr1 = (char *)cc_isa_encptr((uint64_t)la, &md);
    strncpy(ptr1_view, ptr1, sizeof("Hello World\n"));
    ptr1_view[sizeof("Hello World\n") - 1] = 0;

    md.version_ = version++ % VERSION_SIZE;
    auto ptr2 = (char *)cc_isa_encptr((uint64_t)la, &md);
    strncpy(ptr2_view, ptr2, sizeof("Hello World\n"));
    ptr2_view[sizeof("Hello World\n") - 1] = 0;

    md.version_ = version++ % VERSION_SIZE;
    auto ptr3 = (char *)cc_isa_encptr((uint64_t)la, &md);
    strncpy(ptr3_view, ptr3, sizeof("Hello World\n"));
    ptr3_view[sizeof("Hello World\n") - 1] = 0;

    dbgprint("%-10s %s", "ptr1", ptr1_view);
    dbgprint("%-10s %s", "ptr2", ptr2_view);
    dbgprint("%-10s %s", "ptr3", ptr3_view);

    ASSERT_STRNE(ptr, ptr1_view);
    ASSERT_STRNE(ptr, ptr2_view);
    ASSERT_STRNE(ptr, ptr3_view);
    ASSERT_STRNE(ptr1_view, ptr2_view);
    ASSERT_STRNE(ptr1_view, ptr3_view);
    ASSERT_STRNE(ptr2_view, ptr3_view);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
