cmake_minimum_required(VERSION 3.0.0)
include(../../spec-prologue.cmake)
project(${extracted_proj_nm})
include(../../spec.cmake)

include_directories(include)
add_compile_options(-include string.h -include stdlib.h -include cstddef)
add_compile_definitions(BOOST_DISABLE_THREADS deal_II_dimension=3)
