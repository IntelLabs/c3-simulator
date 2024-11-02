#                                                              -*- Makefile -*-
# Simics module makefile
#

project_dir := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../..
common_dir := $(project_dir)/modules/common
c3lib_dir := $(project_dir)/c3lib
include $(common_dir)/Makefile.common.mk

PYTHON_FILES = module_load.py simics_start.py

MODULE_CLASSES = riscv_c3_model riscv_c3_model_connection
SRC_FILES += riscv_c3_simics_module.cpp
SRC_FILES += common/crypto/ascon_cipher.cpp
SRC_FILES += common/crypto/bipbip.cpp

EXTRA_OBJ_FILES += $(XED_LIB)
MODULE_CFLAGS += $(XED_CFLAGS) -DC3_RISC_V

MODULE_CFLAGS += -Werror
MODULE_CFLAGS += -Wall
ifeq ($(D),1)
	MODULE_CFLAGS += -g -O0 -fno-inline
else
	MODULE_CFLAGS += -O3
endif
MODULE_CFLAGS += -I$(common_dir)
MODULE_CFLAGS += -I$(c3lib_dir)

SIMICS_API := latest
THREAD_SAFE = yes
include $(MODULE_MAKEFILE)
