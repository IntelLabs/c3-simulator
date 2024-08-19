# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

ifneq ("$(CC)","")
ifeq (12, $(shell $(CC) --version | head -n1 | grep gcc | awk '{print $3}' | sed 's/\..*//'))
GCC12 = $(CC)
endif
endif

ifeq ("$(GCC12)","")
ifneq ("$(wildcard /opt/simics/GCC-12.2.0/bin/gcc)", "")
GCC12 = /opt/simics/GCC-12.2.0/bin/gcc
endif
endif

ifeq ("$(GCC12)","")
ifneq ("$(wildcard $(project_dir)/lib/gcc12/install/bin/gcc)","")
GCC12 = $(project_dir)/lib/gcc12/install/bin/gcc
endif
endif


CCACHE := $(shell command -v ccache 2> /dev/null)
LINUX_CCACHE_DIR := ${HOME}/.c3-linux-ccache
ifneq ("$(CCACHE)","")
LINUX_MAKE_ARGS = CC="ccache $(GCC12)" CCACHE_DIR=$(LINUX_CCACHE_DIR)
else
LINUX_MAKE_ARGS = CC="$(LINUX_CC)"
endif

LINUX_MAKE_ARGS += EXTRA_CFLAGS="-I$(project_dir)/c3lib"

install_dependencies_ubuntu::
	$(info === Dependencies for Linux builds)
	sudo apt install -y \
		build-essential \
		bison \
		dwarves \
		flex \
		libelf-dev \
		libssl-dev \
		llvm \
		zstd

linux/src/.config: linux/configs/default_config
	cp -f $< $@

.PHONY: linux-config
linux-config: linux/src/.config
ifneq (n,$(findstring n,$(firstword -$(MAKEFLAGS))))
	$(MAKE) -C linux/src oldconfig $(LINUX_MAKE_ARGS)
	$(MAKE) -C linux/src prepare $(LINUX_MAKE_ARGS)
	$(MAKE) -C linux/src scripts $(LINUX_MAKE_ARGS)
else
	# $(MAKE) -C linux/src oldconfig $(LINUX_MAKE_ARGS)
	# $(MAKE) -C linux/src prepare $(LINUX_MAKE_ARGS)
	# $(MAKE) -C linux/src scripts $(LINUX_MAKE_ARGS)
endif

.PHONY: linux-oldconfig
linux-oldconfig: linux/src/.config
ifneq (n,$(findstring n,$(firstword -$(MAKEFLAGS))))
	yes "" | $(MAKE) -C linux/src oldconfig $(LINUX_MAKE_ARGS)
else
	# yes "" | $(MAKE) -C linux/src oldconfig $(LINUX_MAKE_ARGS)
endif

.PHONY: make_linux
make_linux: linux-oldconfig
	$(info === Build Linux kernel)
ifneq (n,$(findstring n,$(firstword -$(MAKEFLAGS))))
	$(MAKE) -C linux/src $(LINUX_MAKE_ARGS)
else
	# $(MAKE) -C linux/src $(LINUX_MAKE_ARGS)
endif


linux/linux.tar.gz: make_linux
	$(info === Package Linux kernel)
	rm -f $@
	cd $(dir $@) && tar czf $(notdir $@) src

.PHONY: linux-mrproper
linux-mrproper:
ifneq (n,$(findstring n,$(firstword -$(MAKEFLAGS))))
	$(MAKE) -C linux/src mrproper
else
	# $(MAKE) -C linux/src mrproper
endif

.PHONY: mrproper
mrproper:: linux-mrproper

.PHONY: linux
linux: c3_docker-make_linux
