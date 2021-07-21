cmake_minimum_required(VERSION 3.0.0)
include(../../spec-prologue.cmake)
project(${extracted_proj_nm})
include(../../spec.cmake)

add_compile_options(-std=c99 -fopenmp)
add_compile_definitions(LARGE_WORKLOAD)
