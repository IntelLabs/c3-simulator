cmake_minimum_required(VERSION 3.0.0)
include(../../spec-prologue.cmake)
project(${extracted_proj_nm})
include(../../spec.cmake)

include_directories(.)
add_compile_definitions(FN FAST CONGRAD_TMP_VECTORS DSLASH_TMP_LINKS SPEC_CPU_LINUX)
