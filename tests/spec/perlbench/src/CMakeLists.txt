cmake_minimum_required(VERSION 3.0.0)
include(../../spec-prologue.cmake)
project(${extracted_proj_nm})
set(src_files av.c deb.c doio.c doop.c dump.c globals.c gv.c hv.c locale.c mg.c numeric.c op.c pad.c perl.c perlapi.c perlio.c perlmain.c perly.c pp.c pp_ctl.c pp_hot.c pp_pack.c pp_sort.c pp_sys.c regcomp.c regexec.c run.c scope.c sv.c taint.c toke.c universal.c utf8.c util.c xsutils.c Base64.c Cwd.c Dumper.c HiRes.c IO.c Peek.c attrs.c poll.c stdio.c DynaLoader.c MD5.c Storable.c Parser.c specrand.c Hostname.c Opcode.c)
include(../../spec.cmake)

add_compile_options(-std=gnu89)
add_compile_definitions(PERL_CORE SPEC_CPU_LINUX_X64)
