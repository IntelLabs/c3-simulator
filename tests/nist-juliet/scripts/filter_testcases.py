#!/usr/bin/env python3
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

import os
import sys
import re

irrelevant_testcases={}

irrelevant_testcases ["CWE122_Heap_Based_Buffer_Overflow/s01"] = [
                        "CWE122_Heap_Based_Buffer_Overflow/s01/CWE122_Heap_Based_Buffer_Overflow__char_type_overrun_memcpy_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s01/CWE122_Heap_Based_Buffer_Overflow__char_type_overrun_memmove_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s01/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE129_fgets_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s01/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE129_fscanf_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s01/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE129_large_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s01/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE193_char_cpy_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s01/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE129_rand_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s01/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE193_char_loop_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s01/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE193_char_memcpy_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s01/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE193_char_memmove_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s01/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE193_char_ncpy_12.cpp"
                        ]

irrelevant_testcases ["CWE122_Heap_Based_Buffer_Overflow/s02"] = [
                        "CWE122_Heap_Based_Buffer_Overflow/s02/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_char_loop_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s02/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_char_memcpy_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s02/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_char_memmove_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s02/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_char_ncat_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s02/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_char_ncpy_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s02/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_char_snprintf_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s02/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_class_loop_12.cpp"
                    ]

irrelevant_testcases ["CWE122_Heap_Based_Buffer_Overflow/s03"] = [
                        "CWE122_Heap_Based_Buffer_Overflow/s03/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_class_memcpy_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s03/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_class_memmove_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s03/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_int_loop_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s03/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_int_memcpy_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s03/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_int_memmove_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s03/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_int64_t_loop_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s03/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_int64_t_memcpy_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s03/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE805_int64_t_memmove_12.cpp"
                    ]

irrelevant_testcases ["CWE122_Heap_Based_Buffer_Overflow/s04"] = [
                        "CWE122_Heap_Based_Buffer_Overflow/s04/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE806_char_loop_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s04/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE806_char_memcpy_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s04/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE806_char_memmove_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s04/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE806_char_ncat_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s04/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE806_char_ncpy_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s04/CWE122_Heap_Based_Buffer_Overflow__cpp_CWE806_char_snprintf_12.cpp"
                    ]

irrelevant_testcases ["CWE122_Heap_Based_Buffer_Overflow/s05"] = [
                        "CWE122_Heap_Based_Buffer_Overflow/s05/CWE122_Heap_Based_Buffer_Overflow__cpp_dest_char_cat_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s05/CWE122_Heap_Based_Buffer_Overflow__cpp_dest_char_cpy_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s05/CWE122_Heap_Based_Buffer_Overflow__cpp_src_char_cat_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s05/CWE122_Heap_Based_Buffer_Overflow__cpp_src_char_cpy_12.cpp",
                        "CWE122_Heap_Based_Buffer_Overflow/s05/CWE122_Heap_Based_Buffer_Overflow__CWE131_loop_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s05/CWE122_Heap_Based_Buffer_Overflow__CWE131_memcpy_12.c"
                    ]

irrelevant_testcases ["CWE122_Heap_Based_Buffer_Overflow/s06"] = [
                        "CWE122_Heap_Based_Buffer_Overflow/s06/CWE122_Heap_Based_Buffer_Overflow__c_CWE129_fgets_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s06/CWE122_Heap_Based_Buffer_Overflow__c_CWE129_fscanf_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s06/CWE122_Heap_Based_Buffer_Overflow__c_CWE129_large_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s06/CWE122_Heap_Based_Buffer_Overflow__c_CWE193_char_cpy_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s06/CWE122_Heap_Based_Buffer_Overflow__c_CWE193_char_loop_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s06/CWE122_Heap_Based_Buffer_Overflow__c_CWE193_char_memcpy_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s06/CWE122_Heap_Based_Buffer_Overflow__CWE131_memmove_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s06/CWE122_Heap_Based_Buffer_Overflow__CWE135_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s06/CWE122_Heap_Based_Buffer_Overflow__c_CWE129_rand_12.c"
                    ]
irrelevant_testcases ["CWE122_Heap_Based_Buffer_Overflow/s07"] = [
                        "CWE122_Heap_Based_Buffer_Overflow/s07/CWE122_Heap_Based_Buffer_Overflow__c_CWE193_char_memmove_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s07/CWE122_Heap_Based_Buffer_Overflow__c_CWE193_char_ncpy_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s07/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_char_loop_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s07/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_char_memcpy_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s07/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_char_memmove_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s07/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_char_ncat_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s07/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_char_ncpy_12.c"
                    ]
irrelevant_testcases ["CWE122_Heap_Based_Buffer_Overflow/s08"] = [
                        "CWE122_Heap_Based_Buffer_Overflow/s08/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_char_snprintf_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s08/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_int_loop_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s08/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_int_memcpy_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s08/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_int_memmove_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s08/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_int64_t_loop_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s08/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_int64_t_memcpy_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s08/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_int64_t_memmove_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s08/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_struct_loop_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s08/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_struct_memcpy_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s08/CWE122_Heap_Based_Buffer_Overflow__c_CWE805_struct_memmove_12.c"
                    ]
irrelevant_testcases ["CWE122_Heap_Based_Buffer_Overflow/s09"] = [
                        "CWE122_Heap_Based_Buffer_Overflow/s09/CWE122_Heap_Based_Buffer_Overflow__c_CWE806_char_loop_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s09/CWE122_Heap_Based_Buffer_Overflow__c_CWE806_char_memcpy_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s09/CWE122_Heap_Based_Buffer_Overflow__c_CWE806_char_memmove_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s09/CWE122_Heap_Based_Buffer_Overflow__c_CWE806_char_ncat_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s09/CWE122_Heap_Based_Buffer_Overflow__c_CWE806_char_ncpy_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s09/CWE122_Heap_Based_Buffer_Overflow__c_CWE806_char_snprintf_12.c"
                    ]
irrelevant_testcases ["CWE122_Heap_Based_Buffer_Overflow/s10"] = [
                        "CWE122_Heap_Based_Buffer_Overflow/s10/CWE122_Heap_Based_Buffer_Overflow__c_dest_char_cat_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s10/CWE122_Heap_Based_Buffer_Overflow__c_dest_char_cpy_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s10/CWE122_Heap_Based_Buffer_Overflow__c_src_char_cat_12.c",
                        "CWE122_Heap_Based_Buffer_Overflow/s10/CWE122_Heap_Based_Buffer_Overflow__c_src_char_cpy_12.c"
                    ]
irrelevant_testcases ["CWE122_Heap_Based_Buffer_Overflow/s11"] = [ "*" ]

irrelevant_testcases ["CWE416_Use_After_Free"] = [
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__malloc_free_char_12.c",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__malloc_free_int_12.c",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__malloc_free_int64_t_12.c",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__malloc_free_long_12.c",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__malloc_free_struct_12.c",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__new_delete_array_char_12.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__new_delete_array_class_12.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__new_delete_array_int_12.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__new_delete_array_int64_t_12.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__new_delete_array_long_12.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__new_delete_array_struct_12.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__new_delete_char_12.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__new_delete_class_12.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__new_delete_int_12.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__new_delete_int64_t_12.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__new_delete_long_12.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__new_delete_struct_12.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__operator_equals_01_good1.cpp",
                        "CWE416_Use_After_Free/CWE416_Use_After_Free__return_freed_ptr_12.c"
                    ]
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
irrelevant_testcases ["CWE457_Use_of_Uninitialized_Variable/s02"] = [
                        "CWE457_Use_of_Uninitialized_Variable/s02/CWE457_Use_of_Uninitialized_Variable__twointsclass_array_malloc_partial_init_62b.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s02/CWE457_Use_of_Uninitialized_Variable__twointsclass_array_malloc_partial_init_63b.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s02/CWE457_Use_of_Uninitialized_Variable__twointsclass_array_malloc_partial_init_64b.cpp",
                        "CWE457_Use_of_Uninitialized_Variable/s02/CWE457_Use_of_Uninitialized_Variable__twointsclass_array_new_*"
                        ]

def filterOutTestcases(workloadDir, binariesInDir):
    print("Found " + str(len(binariesInDir)) + " binaries")

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

    sourcesToFilter = []
    for b in irrelevant_binaries:
        if "*" in b:
            pattern=''
            temp=str.partition(b, "*")
            if temp[0] == '':
                pattern = temp[2]
            else:
                pattern = temp[0]
            temp=str.partition(pattern, "*")
            pattern = temp[0]

            sourcesToFilter+=[i for i in binariesInDir if pattern in i]

        else:
            sourcesToFilter+=[output[0] + b]

    binariesToFilter=[re.sub('[.][c](p{2})*', '.out', s) for s in sourcesToFilter]
    print(str(len(binariesToFilter)) + " testcases to filter:")

    binariesToRun = list(set(binariesInDir) - set(binariesToFilter))
    print(str(len(binariesToRun)) + " testcases to run:")
    [print(f) for f in binariesToRun]

    return binariesToRun
