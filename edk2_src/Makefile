# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

include ../config-user.mk

EDK2_SRC := $(project_dir)/edk2_src
EDK2_SCRIPTS = $(EDK2_SRC)/scripts
EDK2_INIT_DEPS = $(EDK2_SRC)/edk2/Conf

EDK2_BUILDROOT_SSH_IP ?= 127.0.0.1
EDK2_BUILDROOT_SSH_PORT ?= 4022
EDK2_BUILDROOT_SSH_USER ?= simics
EDK2_BUILDROOT_SSH_ARGS += -o StrictHostKeyChecking=no
EDK2_BUILDROOT_SSH_ARGS += -o UserKnownHostsFile=/dev/null
EDK2_BUILDROOT_SSH_ARGS += -p $(EDK2_BUILDROOT_SSH_PORT)
EDK2_BUILDROOT_SSH_ARGS += $(EDK2_BUILDROOT_SSH_USER)@$(EDK2_BUILDROOT_SSH_IP)

EXTRA_DOCKER_MAKE_ARGS += EDK2_MODULE_CONFIG="$(EDK2_MODULE_CONFIG)"

EDK2_RUN_FLAGS += --enable_integrity

ifeq (n,$(findstring n,$(firstword -$(MAKEFLAGS))))
	EDK2_RUN_FLAGS += -n
endif

.PHONY: all
all: debug

EDK2_GIT_SUBMODULES = $(EDK2_SRC)/edk2 $(EDK2_SRC)/edk2-platforms
EDK2_GIT_SUBMODULES += $(EDK2_SRC)/edk2-non-osi $(EDK2_SRC)/FSP

EDK2_GIT_SUBMODULES_GITDIR = $(addsuffix /.git, $(EDK2_GIT_SUBMODULES))

$(EDK2_GIT_SUBMODULES_GITDIR):
	git submodule update --init --recursive $(shell dirname $@)

.PHONY: git_init
git_init: $(EDK2_GIT_SUBMODULES_GITDIR)

.PHONY: init
init: git_init
	mkdir -p $(EDK2_SRC)/edk2/Conf
	+ (cd $(EDK2_SRC)/edk2 && make -C BaseTools)
	+ (cd $(EDK2_SRC)/edk2 && bash -c "source ./edksetup.sh")

.PHONY: debug
debug: $(EDK2_INIT_DEPS)
	+ edk2_module_config=$(EDK2_MODULE_CONFIG) $(EDK2_SCRIPTS)/build_uefi_simics.sh

.PHONY: release
release: $(EDK2_INIT_DEPS)
	+ debug=0 edk2_module_config=$(EDK2_MODULE_CONFIG) $(EDK2_SCRIPTS)/build_uefi_simics.sh

.PHONY: buildroot-setup
buildroot-setup:
	+ $(EDK2_SCRIPTS)/edk2_run_linux.sh setup $(EDK2_RUN_FLAGS)

.PHONY: buildroot-config
buildroot-config:
	+ $(EDK2_SCRIPTS)/edk2_run_linux.sh buildroot-config $(EDK2_RUN_FLAGS)

.PHONY: buildroot-build
buildroot-build:
	+ $(EDK2_SCRIPTS)/edk2_run_linux.sh build $(EDK2_RUN_FLAGS)

.PHONY: buildroot
buildroot: buildroot-setup
	+ $(EDK2_SCRIPTS)/edk2_run_linux.sh build $(EDK2_RUN_FLAGS)

.PHONY: buildroot-ssh
buildroot-ssh:
	ssh $(EDK2_BUILDROOT_SSH_ARGS)

.PHONY: buildroot-run
buildroot-run:
	exec $(EDK2_SCRIPTS)/edk2_run_linux.sh run --net $(EDK2_RUN_FLAGS)

.PHONY: buildroot-run-and-quit
buildroot-run-and-quit:
	exec $(EDK2_SCRIPTS)/edk2_run_linux.sh run --batch-mode --do_quit $(EDK2_RUN_FLAGS)

.PHONY: clean
clean::
ifneq ($(wildcard $(EDK2_SRC)/edk2/BaseTools/*),)
	make -C $(EDK2_SRC)/edk2/BaseTools clean
endif
	rm -rf $(EDK2_SRC)/edk2/Conf
	rm -rf $(EDK2_SRC)/Build/*

.PHONY: mrproper
mrproper:: clean
	rm -f $(EDK2_SRC)/Build/* \
	      $(EDK2_SRC)/edk2-platforms/Silicon/Intel/Tools/FitGen/FitGen.d \
	      $(EDK2_SRC)/edk2-platforms/Silicon/Intel/Tools/FitGen/FitGen.o
ifneq ($(wildcard $(EDK2_SRC)/edk2-platforms/Platform/Intel/*),)
ifeq ($(INSIDE_DOCKER),true)
	$(warning ############################################################)
	$(warning # Please run edk2-mrproper outside docker for full cleanup #)
	$(warning ############################################################)
else
	cd $(EDK2_SRC)/edk2-platforms/Platform/Intel && ./build_bios.py --cleanall;
	git submodule deinit $(EDK2_SRC)/edk2-platforms;
	git submodule update --init --recursive $(EDK2_SRC)/edk2-platforms;
endif
endif
