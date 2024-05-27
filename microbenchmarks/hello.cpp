/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#include <assert.h>
#include <unistd.h>
#include <iostream>

void hello_stack() {
    int stack_i = 1;
    int stack_j = 2;
    int stack_k = 3;
    printf("%-20s %016lx\n", "stack_i", (uint64_t)&stack_i);
    printf("%-20s %016lx\n", "stack_j", (uint64_t)&stack_j);
    printf("%-20s %016lx\n", "stack_k", (uint64_t)&stack_k);
}

int main() {
    std::cout << "Hello World" << std::endl;
    uint32_t *p = reinterpret_cast<uint32_t *>(malloc(4));
    uint32_t *q = reinterpret_cast<uint32_t *>(malloc(4));
    uint32_t *r = reinterpret_cast<uint32_t *>(malloc(4));
    uint64_t *q64;

    *p = 0xdeadbeef;
    *q = 0xabcd1234;
    *r = 0xfafa5555;
    *q = 0xcafecafe;

    printf("p addr=0x%016lx\n", (uint64_t)p);
    printf("q addr=0x%016lx\n", (uint64_t)q);
    printf("r addr=0x%016lx\n", (uint64_t)r);
    printf("p        = 0x%08x\n", *p);
    printf("q        = 0x%08x\n", *q);
    printf("r        = 0x%08x\n", *r);

    q = reinterpret_cast<uint32_t *>(realloc(q, 8));
    printf("q addr (realloc)=0x%016lx\n", (uint64_t)q);
    printf("q (realloc)     = 0x%08x\n", *q);
    printf("q (realloc, u64)= 0x%016lx\n", *reinterpret_cast<uint64_t *>(q));
    assert(0xcafecafe == *q && "ERROR: q value mismatch after realloc\n");

    uint64_t check_value = *reinterpret_cast<uint64_t *>(q);

    q = reinterpret_cast<uint32_t *>(realloc(q, 64));
    printf("q addr (realloc)=0x%016lx\n", (uint64_t)q);
    printf("q (realloc)     = 0x%08x\n", *q);
    if (*q != 0xcafecafe)
        printf("ERROR: q value mismatch after realloc\n");
    assert(0xcafecafe == *q && "ERROR: q value mismatch after realloc\n");
    assert(check_value == *reinterpret_cast<uint64_t *>(q) &&
           "ERROR: q uint64 value mismatch after realloc");

    free(p);
    free(q);
    free(r);

    hello_stack();
    return 0;
}
