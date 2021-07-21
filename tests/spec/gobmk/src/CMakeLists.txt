cmake_minimum_required(VERSION 3.0.0)
include(../../spec-prologue.cmake)
project(${extracted_proj_nm})
include(../../spec.cmake)

add_compile_options(-std=gnu89)
include_directories(. .. ../include ./include)
add_compile_definitions(HAVE_CONFIG_H)
