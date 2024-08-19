# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

import os
import sys
import re

irrelevant_testcases={}
irrelevant_testcases ["CWE457_Use_of_Uninitialized_Variable/s01"] = [
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__char_pointer_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_0*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_1*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_43.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_6*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_array_alloca_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_array_declare_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_array_malloc_no_init_62b.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_array_malloc_no_init_63b.c",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_array_malloc_no_init_64b.c",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_array_malloc_partial_init_62b.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_array_malloc_partial_init_63b.c",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_array_malloc_partial_init_64b.c",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__double_pointer_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__empty_constructor_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_0*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_1*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_43.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_6*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_array_alloca_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_array_declare_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_array_malloc_no_init_62b.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_array_malloc_no_init_63b.c",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_array_malloc_no_init_64b.c",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_array_malloc_partial_init_62b.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_array_malloc_partial_init_63b.c",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_array_malloc_partial_init_64b.c",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int_pointer_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__int64_t_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__long_0*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__long_1*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__long_43.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__long_6*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__new_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__no_constructor_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_0*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_1*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_43.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_6*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_array_alloca_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_array_declare_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_array_malloc_no_init_62b.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_array_malloc_no_init_63b.c",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_array_malloc_no_init_64b.c",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_array_malloc_partial_init_62b.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_array_malloc_partial_init_63b.c",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_array_malloc_partial_init_64b.c",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__struct_pointer_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__twointsclass_0*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__twointsclass_1*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__twointsclass_43.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__twointsclass_6*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__twointsclass_array_alloca_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__twointsclass_array_declare_*",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__twointsclass_array_malloc_no_init_62b.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__twointsclass_array_malloc_no_init_63b.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s01/CWE457_Use_of_Uninitialized_Variable__twointsclass_array_malloc_no_init_64b.cpp"
                        ]

def enumerateBinariesInDir(path):
    ret = []
    isDir = -1
    if os.path.isfile(path):
        ret.append(path)
        isDir = 0
    elif os.path.isdir(path):
        isDir = 1
        for dirpath, dirs, files in os.walk(path):
            for fl in files:
                #if fl.endswith(".out"): # Filter only *.out
                ret.append(os.path.join(dirpath, fl))
    return ret, isDir

def filterOutTestcases(workloadDir, binariesInDir):
    output = str.partition(workloadDir, "CWE")
    dictKey = output[1] + output[2]
    if dictKey[-1] == '/':
        dictKey = dictKey[:-1]

    print("Workload directory: " + dictKey)

    if irrelevant_testcases.get(dictKey) == None:
        return binariesInDir

    irrelevant_binaries = irrelevant_testcases[dictKey]
    if irrelevant_binaries[0] == "*":
        print("Ignore the whole directory of testcases!")
        return []

    binariesToFilter = []
    for b in irrelevant_binaries:
        if "*" in b:
            pattern=''
            temp=str.partition(b, "*")
            #pattern = output[0] + temp[0]
            if temp[0] == '':
                pattern = temp[2]
            else:
                pattern = temp[0]
            temp=str.partition(pattern, "*")
            pattern = temp[0]

            binariesToFilter+=[i for i in binariesInDir if pattern in i]

        else:
            binariesToFilter+=[output[0] + b]

    binariesToFilter=[re.sub('[.][c](p{2})*', '.out', s) for s in binariesToFilter]
    print(str(len(binariesToFilter)) + " testcases to filter:")
    #[print(f) for f in binariesToFilter]

    #print("\nTestcases to ignore:")
    #[print(f) for f in binariesToFilter]

    binariesToRun = list(set(binariesInDir) - set(binariesToFilter))

    print(str(len(binariesToRun)) + " testcases to run:")
    #[print(f) for f in binariesToRun]
    for i in range(25):
        print(binariesToFilter[i])

    return binariesToRun

#workloadDir="/home/salminsu/pil-ssultana/NIST_Juliet/c3-release-juliet/tests/nist-juliet/test-suite/testcases/CWE457_Use_of_Uninitialized_Variable/s01/"
workloadDir="/home/salminsu/pil-ssultana/NIST_Juliet/c3-release-juliet/tests/nist-juliet/unit-tests/testcases/CWE122_Heap_Based_Buffer_Overflow_Stack"
binariesToRun, isDir = enumerateBinariesInDir(workloadDir)
print("Found " + str(len(binariesToRun)) + " binaries")
if isDir == 1:
    binariesToRun = filterOutTestcases(workloadDir, binariesToRun)

[print(f) for f in binariesToRun]

'''
b="pointer"
pattern=''
if "*" in b:
    temp=str.partition(b, "*")
    #pattern = output[0] + temp[0]
    if temp[0] == '':
        pattern = temp[2]
    else:
        pattern = temp[0]
    temp=str.partition(pattern, "*")
    pattern = temp[0]
else:
    pattern=b
print(pattern)
'''
