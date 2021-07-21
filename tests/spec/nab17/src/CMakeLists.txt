cmake_minimum_required(VERSION 3.0.0)
include(../../spec-prologue.cmake)
project(${extracted_proj_nm})
set(src_files nabmd.c sff.c nblist.c prm.c memutil.c molio.c molutil.c errormsg.c binpos.c rand2.c select_atoms.c reslib.c database.c traceback.c chirvol.c specrand/specrand.c regex-alpha/regcomp.c regex-alpha/regerror.c regex-alpha/regexec.c regex-alpha/regfree.c)
include(../../spec.cmake)

add_compile_options(-std=c99 -fopenmp)
add_compile_definitions(NOPERFLIB NOREDUCE)
include_directories(specrand regex-alpha)
