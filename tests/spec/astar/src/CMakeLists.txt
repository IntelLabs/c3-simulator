cmake_minimum_required(VERSION 3.0.0)
include(../../spec-prologue.cmake)
project(${extracted_proj_nm})
include(../../spec.cmake)
include(../../spec06.cmake)

add_compile_definitions(SPEC_CPU_LITTLE_ENDIAN)
