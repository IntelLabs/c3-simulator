# Copyright 2023-2024 Intel Corporation
# SPDX-License-Identifier: MIT

BUILDROOT_RUN_FLAGS += --enable_c3 --batch-mode --fcmd 'dmesg | tail -n 5'

.PHONY: linux_buildroot
linux_buildroot:
	+ $(MAKE) $(MAKEFLAG_N) linux_buildroot_setup
	+ $(MAKE) $(MAKEFLAG_N) linux_buildroot_prepare
	+ $(MAKE) $(MAKEFLAG_N) linux_buildroot_build
	+ $(MAKE) $(MAKEFLAG_N) linux_buildroot_run

.PHONY: linux_buildroot_setup
linux_buildroot_setup:
	$(project_dir)/scripts/run_buildroot.sh setup

.PHONY: linux_buildroot_prepare
linux_buildroot_prepare:
	cp $(project_dir)/linux/configs/default_config_kernel_heap $(project_dir)/linux/src/.config
	make olddefconfig -C $(project_dir)/linux/src
	make prepare -C $(project_dir)/linux/src

.PHONY: linux_buildroot_build
linux_buildroot_build:
	LIBS= $(project_dir)/scripts/run_buildroot.sh build

.PHONY: linux_buildroot_run
linux_buildroot_run:
	$(MAKE)
	exec $(project_dir)/scripts/run_buildroot.sh run $(BUILDROOT_RUN_FLAGS)

.PHONY: linux_buildroot_mrproper
linux_buildroot_mrproper:
	rm -rf $(project_dir)/buildroot
