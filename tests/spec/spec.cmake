add_executable(${PROJECT_NAME})
if(NOT DEFINED src_files)
    execute_process(COMMAND /usr/bin/find ( -name *.c -o -name *.C -o -name *.cc -o -name *.cpp ) -printf "%p;" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE src_files)
endif()
target_sources(${PROJECT_NAME} PRIVATE ${src_files})
target_link_libraries(${PROJECT_NAME} -lm)
add_compile_options(-m64)

string(LENGTH ${PROJECT_NAME} proj_nm_len)
set(specver_len 2)
math(EXPR proj_nm_specver_offset "${proj_nm_len} - ${specver_len}")
string(SUBSTRING ${PROJECT_NAME} ${proj_nm_specver_offset} ${specver_len} proj_nm_sfx)
if(${proj_nm_sfx} STREQUAL "17")
    include(../../spec17.cmake)
else()
    include(../../spec06.cmake)
endif()
