# Copyright 2022-2025 Intel Corporation
# SPDX-License-Identifier: MIT

# glibc-noshim

.PHONY: make_glibc-noshim
make_glibc-noshim:
	$(info === Build $@)
	+ CC_USE_SYSCALL_SHIMS=0 ./glibc/make_glibc.sh

.PHONY: glibc
glibc: c3_docker-make_glibc-noshim

.PHONY: glibc-noshim
glibc-noshim: c3_docker-make_glibc-noshim

# glibc-shim

.PHONY: make_glibc-shim
make_glibc-shim:
	$(info === Build $@)
	+ CC_USE_SYSCALL_SHIMS=1 ./glibc/make_glibc.sh


.PHONY: glibc-shim
glibc-shim: c3_docker-make_glibc-shim

# glibc-nowrap

.PHONY: make_glibc-nowrap
make_glibc-nowrap:
	$(info === Build $@)
	+ CC_NO_WRAP_ENABLE=1 ./glibc/make_glibc.sh

.PHONY: make_glibc-nowrap
glibc-nowrap: c3_docker-make_glibc-nowrap

# glibc-shim-detect-1b-ovf

.PHONY: make_glibc-shim-detect-1b-ovf
make_glibc-shim-detect-1b-ovf:
	$(info === Build $@)
	+ CC_USE_SYSCALL_SHIMS=1 CC_DETECT_1B_OVF=1 ./glibc/make_glibc.sh

.PHONY: glibc-shim-detect-1b-ovf
glibc-shim-detect-1b-ovf: c3_docker-glibc-shim-detect-1b-ovf

# glibc- noshim-detect-1b-ovf

.PHONY: make_glibc-noshim-detect-1b-ovf
make_glibc-noshim-detect-1b-ovf:
	$(info === Build $@)
	+ CC_USE_SYSCALL_SHIMS=0 CC_DETECT_1B_OVF=1 ./glibc/make_glibc.sh

.PHONY: glibc-noshim-detect-1b-ovf
glibc-noshim-detect-1b-ovf: c3_docker-glibc-noshim-detect-1b-ovf

mrproper::
	rm -rf glibc/glibc-2.30_build glibc/glibc-2.30_install

install_dependencies_ubuntu::
	$(info === glibc dependencies)
	sudo apt install -y \
		gawk
