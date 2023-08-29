#                                                              -*- Makefile -*-
# Simics module makefile
#

project_dir := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../..
common_dir := $(project_dir)/modules/common
include $(common_dir)/Makefile.common.mk

PYTHON_FILES = module_load.py simics_start.py

MODULE_CLASSES = lim_model lim_model_connection
SRC_FILES += lim_simics_module.cpp
SRC_FILES += lim_simics_connection.cpp
SRC_FILES += model.cpp

MODULE_CFLAGS += -Werror
MODULE_CFLAGS += -Wall
ifeq ($(D),1)
	MODULE_CFLAGS += -g -O0 -fno-inline
else
	MODULE_CFLAGS += -O3
endif
MODULE_CFLAGS += -I$(common_dir)

SIMICS_API := latest
THREAD_SAFE = yes

include $(common_dir)/Makefile.common.mk

include $(MODULE_MAKEFILE)
