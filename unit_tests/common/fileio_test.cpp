// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
#include <string.h>
#include <fstream>
#include <iostream>
#include <gtest/gtest.h>
using namespace std;

void write_ostream(std::ostream &out) {
    out << "First line" << endl;
    out << "Second line" << endl;
}

TEST(FileIO, WriteTwoLinesToFile) {
    ofstream myfile("fileio_test1.txt");
    ASSERT_TRUE(myfile.is_open());
    myfile << "First line.\n";
    myfile << "Second line.\n";
    ASSERT_TRUE(myfile.good());
    myfile.close();
}
TEST(FileIO, WriteFromMallocBuffer) {
    ofstream myfile("fileio_test2.txt");
    ASSERT_TRUE(myfile.is_open());
    // char* buffer = new char[100];
    char *buffer = (char *)malloc(100);
    const char str[] = "This is a test line.\n";
    strncpy(buffer, str, 100);
    for (int i = 0; i < 100; i++) {
        myfile << buffer;
    }
    ASSERT_TRUE(myfile.good());
    myfile.close();
}

TEST(FileIO, WriteRandomFromMallocBuffer) {
    ofstream myfile("WriteRandomFromMallocBuffer.txt");
    ASSERT_TRUE(myfile.is_open());
    // char* buffer = new char[100];
    char *buffer;
    size_t max_size = 100000;
    srand(1);
    for (int i = 0; i < 100; i++) {
        size_t size = rand() % max_size;
        buffer = (char *)calloc(1, size);
        for (size_t j = 0; j < size - 2; j++) {
            buffer[j] = 'a' + (j % 26);
        }
        buffer[size - 2] = '\n';
        buffer[size - 1] = '\0';
        myfile << buffer;
        if (!myfile.good()) {
            fprintf(stderr, "ERROR: myfile corrupted\n");
            fprintf(stderr, "i=%d, size=%ld, buffer=%p\n", i, size, buffer);
        }
        ASSERT_TRUE(myfile.good());
        free(buffer);
    }
    myfile.close();
}

TEST(FileIO, WriteOstream) {
    ofstream myfile("fileio_test3.txt");
    write_ostream(myfile);
    ASSERT_TRUE(myfile.good());
    myfile.close();
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
