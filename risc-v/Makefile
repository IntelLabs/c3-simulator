# Copyright 2023-2024 Intel Corporation
# SPDX-License-Identifier: MIT

include ../config-user.mk

RISCV_CC ?= riscv64-linux-gnu-gcc-14
RISCV_CXX ?= riscv64-linux-gnu-g++-14

RISCV_CFLAGS = -DC3_RISC_V -Wall -I$(project_dir)/c3lib -I$(project_simics_dir)/src/include -static
RISCV_CXXFLAGS = -DC3_RISC_V -Wall -I$(project_dir)/c3lib -I$(project_simics_dir)/src/include -static

RISCV_DIR := $(project_dir)/risc-v

RISCV_BR_GIT_URL = https://gitlab.com/buildroot.org/buildroot.git
RISCV_BR_GIT_REV = 2024.02
RISCV_BR_DIR = $(RISCV_DIR)/buildroot

RISCV_BR_DEFCONFIG = simics_simple_riscv_defconfig
RISCV_BR_DEFCONFIG_SRC = $(project_dir)/scripts/buildroot_config/$(RISCV_BR_DEFCONFIG)
RISCV_BR_DEFCONFIG_PATH = $(RISCV_BR_DIR)/configs/$(RISCV_BR_DEFCONFIG)

RISCV_BR_MAKE_ARGS = -C $(RISCV_BR_DIR) $(MAKEFLAG_N)

RISCV_BR_IP ?= 127.0.0.1
RISCV_BR_USER ?= root
RISCV_BR_SSH_PORT ?= 4022
# Allow keys to change
RISCV_BR_SSH_ARGS += -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null

# For cloning Buildroot into $(RISCV_BR_DIR)
$(RISCV_BR_DIR)/.git:
	git clone --depth 1 -b $(RISCV_BR_GIT_REV) $(RISCV_BR_GIT_URL) $(RISCV_BR_DIR)

# For generating environment-specific configuration in $(RISCV_BR_DIR)/configs
$(RISCV_BR_DEFCONFIG_PATH): $(RISCV_BR_DIR)/.git $(RISCV_BR_DEFCONFIG_SRC)
	sed 's|#C3_ROOT#|$(project_dir)|' $(RISCV_BR_DEFCONFIG_SRC) > $@

# Run buildroot configure in the Buildroot folder
$(RISCV_BR_DIR)/.config: $(RISCV_BR_DEFCONFIG_PATH)
	+ $(MAKE) -C $(RISCV_BR_DIR) $(MAKEFLAG_N) $(RISCV_BR_DEFCONFIG)

# Force re-configure via buildroot-config
.PHONY: buildroot-config
buildroot-config: $(RISCV_BR_DIR)/.config
	+ $(MAKE) -C $(RISCV_BR_DIR) $(MAKEFLAG_N) $(RISCV_BR_DEFCONFIG)

.PHONY: buildroot-build
buildroot-build: $(RISCV_BR_DIR)/.config
	+ $(MAKE) $(RISCV_BR_MAKE_ARGS)

.PHONY: buildroot-clean
buildroot-clean:
	rm -f $(RISCV_BR_DEFCONFIG_PATH)
	+ $(MAKE) $(RISCV_BR_MAKE_ARGS) clean

.PHONY: buildroot-distclean
buildroot-distclean: buildroot-clean
	+ $(MAKE) $(RISCV_BR_MAKE_ARGS) distclean

SIMICS_RISCV_CPU = $(project_simics_root)/simics-risc-v-cpu-6.0.67
ifneq ($(shell [ -e ${SIMICS_RISCV_CPU} ] && echo 1),1)
SIMICS_RISCV_CPU = $(project_simics_root)/simics-risc-v-cpu-6.0.57
endif
ifneq ($(shell [ -e ${SIMICS_RISCV_CPU} ] && echo 1),1)
SIMICS_RISCV_CPU = $(shell ls -d -1 $(project_simics_root)/simics-risc-v-cpu-* | sort -V | tail -n 1)
endif

SIMICS_RISCV_SIMPLE = $(project_simics_root)/simics-risc-v-simple-6.0.8
ifneq ($(shell [ -e ${SIMICS_RISCV_SIMPLE} ] && echo 1),1)
SIMICS_RISCV_SIMPLE = $(project_simics_root)/simics-risc-v-simple-6.0.4
endif
ifneq ($(shell [ -e ${SIMICS_RISCV_SIMPLE} ] && echo 1),1)
SIMICS_RISCV_SIMPLE= $(shell ls -d -1 $(project_simics_root)/simics-risc-v-simple-* | sort -V | tail -n 1)
endif

.PHONY: configure_simics
configure_simics:
	$(project_dir)/bin/addon-manager -C -b \
		-s $(SIMICS_RISCV_CPU) \
		-s $(SIMICS_RISCV_SIMPLE)

.PHONY: buildroot-distclean
mrproper:: buildroot-distclean

.PHONY: run
run:
	$(project_dir)/simics $(project_dir)/scripts/riscv_common.simics

.PHONY: ssh
ssh:
	ssh $(RISCV_BR_SSH_ARGS) -p $(RISCV_BR_SSH_PORT) $(RISCV_BR_USER)@$(RISCV_BR_IP)

scp-%: | ../%
	scp $(RISCV_BR_SSH_ARGS) $< scp://$(RISCV_BR_USER)@$(RISCV_BR_IP):$(RISCV_BR_SSH_PORT)/$<

hello_riscv.out: hello_riscv.c
	$(RISCV_CC) $(RISCV_CFLAGS) $< -o $@

hello_riscv_c3.out: hello_riscv_c3.cpp bipbip.o ascon_cipher.o
	$(RISCV_CXX) $(RISCV_CXXFLAGS) $^ -o $@

bipbip.o: ../c3lib/c3/crypto/bipbip.cpp
	$(RISCV_CXX) $(RISCV_CXXFLAGS) -c -o $@ $^

ascon_cipher.o: ../c3lib/c3/crypto/ascon_cipher.cpp
	$(RISCV_CXX) $(RISCV_CXXFLAGS) -c -o $@ $^

.PHONY: hello_riscv
hello_riscv: hello_riscv.out
	scp $(RISCV_BR_SSH_ARGS) $< scp://$(RISCV_BR_USER)@$(RISCV_BR_IP):$(RISCV_BR_SSH_PORT)/$<
	ssh $(RISCV_BR_SSH_ARGS) -p $(RISCV_BR_SSH_PORT) $(RISCV_BR_USER)@$(RISCV_BR_IP) ./$<

.PHONY: hello_riscv_c3
hello_riscv_c3: hello_riscv_c3.out
	scp $(RISCV_BR_SSH_ARGS) $< scp://$(RISCV_BR_USER)@$(RISCV_BR_IP):$(RISCV_BR_SSH_PORT)/$<
	ssh $(RISCV_BR_SSH_ARGS) -p $(RISCV_BR_SSH_PORT) $(RISCV_BR_USER)@$(RISCV_BR_IP) ./$<
