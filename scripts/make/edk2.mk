# Copyright 2023 Intel Corporation
# SPDX-License-Identifier: MIT

edk2-%::
	+ env $(SUB_ENV_FLAGS) $(MAKE) -C $(project_dir)/edk2_src $(MAKEFLAG_N) $*

.PHONY: edk2
edk2:
	+ $(MAKE) $(MAKEFLAG_N) edk2-git_init
	+ $(MAKE) $(MAKEFLAG_N) c3_docker-edk2-init
	+ $(MAKE) $(MAKEFLAG_N) c3_docker-edk2-debug

.PHONY: edk2_buildroot
edk2_buildroot:
	+ $(MAKE) $(MAKEFLAG_N) c3_docker-edk2-buildroot

.PHONY: edk2_all
edk2_all: edk2 edk2_buildroot

.PHONY: edk2_run
edk2_run:
	$(MAKE) $(MAKEFLAG_N) edk2-buildroot-run

.PHONY: mrproper
mrproper:: edk2-mrproper
