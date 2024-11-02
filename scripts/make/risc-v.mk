# Copyright 2023-2024 Intel Corporation
# SPDX-License-Identifier: MIT

ifndef SCRIPTS_MAKE_RISC_V_MK_
SCRIPTS_MAKE_RISC_V_MK_=1

riscv-%::
	+ env $(SUB_ENV_FLAGS) $(MAKE) -C $(project_dir)/risc-v $(MAKEFLAG_N) $*

c3_docker::
	mkdir -p risc-v/buildroot

DOCKER_ARGS += --mount type=bind,source="$(project_dir)/risc-v",target="$(workdir)/risc-v"

endif  # SCRIPTS_MAKE_RISC_V_MK_
