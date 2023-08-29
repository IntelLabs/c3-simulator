/*
 Copyright Intel Corporation
 SPDX-License-Identifier: MIT
*/
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "malloc/cc_globals.h"

const char *g_hello = "Hello World\n";

int main(int argc, char **argv) {
    const int str_len = strlen(g_hello);
    auto *ptr = malloc(str_len);
    auto *str = reinterpret_cast<char *>(ptr);

    strncpy(str, g_hello, str_len);

    printf("0x%016lx: %.*s\n", reinterpret_cast<uint64_t>(ptr), str_len, str);

    // MAGIC(0);
    // getchar();

    printf("str: %016lx\n", (uint64_t)str);
    printf("str: %016lx\n", (uint64_t)cc_isa_decptr((uint64_t)str));
    printf("len: %016lx\n", (uint64_t)str_len);
    printf("len: %016lx\n", (uint64_t)str_len * 10);

    str += (str_len - 1);
    cc_isa_invicv((uint64_t)str);
    printf("Going to write to (bad ICV): %016lx\n", (uint64_t)str);
    MAGIC(0);
    *str = 'a';

    str += ((str_len * 9) + 1);
    printf("Going to write to (bad CA-LA): %016lx\n", (uint64_t)str);
    MAGIC(0);
    *str = 'a';

    printf("Done\n");

    free(ptr);
    return 0;
}
