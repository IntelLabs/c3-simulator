cmake_minimum_required(VERSION 3.0.0)
include(../../spec-prologue.cmake)
project(${extracted_proj_nm})
set(src_files classic.c complex.c decoherence.c expn.c gates.c matrix.c measure.c oaddn.c objcode.c omuln.c qec.c qft.c qureg.c shor.c version.c specrand.c)
include(../../spec.cmake)

add_compile_options(-std=gnu89)
add_compile_definitions(SPEC_CPU_LINUX)
