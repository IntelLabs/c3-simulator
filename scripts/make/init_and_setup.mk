# Copyright 2023-2024 Intel Corporation
# SPDX-License-Identifier: MIT

include $(project_dir)/scripts/make/xed.mk

SIMICS_BIN ?= /opt/simics/simics-6/simics-latest/bin

PATCHELF_PKG = patchelf-0.10.tar.gz
PATCHELF_URL = https://codeload.github.com/NixOS/patchelf/tar.gz/refs/tags/0.10
PATCHELF_LOCAL_PATH = /opt/simics/share/$(PATCHELF_PKG)

FILES_TO_DECOMPRESS_Z += $(wildcard internal/spec/x264_17/run/BuckBunny.yuv.zst)
FILES_TO_DECOMPRESS_Z += $(wildcard internal/spec/parest17/run/parest17.zst)
FILES_TO_DECOMPRESS_Z += $(wildcard internal/spec/soplex/run/ref.mps.zst)
FILES_TO_DECOMPRESS = $(FILES_TO_DECOMPRESS_Z:%.zst=%)

.PHONY: simics_setup
simics_setup: patchelf-0.10.tar.gz $(FILES_TO_DECOMPRESS) simics install-xed

simics:
	$(SIMICS_BIN)/project-setup --ignore-existing-files .

$(PATCHELF_PKG):
ifneq ("$(wildcard $(PATCHELF_LOCAL_PATH))","")
	cp $(PATCHELF_LOCAL_PATH) .
else
	curl -o $@ $(PATCHELF_URL)
endif

$(FILES_TO_DECOMPRESS) : % : %.zst
	test -e $@ || zstd -d $^

.PHONY: decompress
decompress: $(FILES_TO_DECOMPRESS)

.PHONY: install-pre-push-hook
install-pre-push-hook:
	[ ! -f .git/hooks/pre-push ] || cp .git/hooks/pre-push .git/hooks/pre-push.backup
	cp scripts/git/hooks/pre-push .git/hooks/pre-push

.PHONY: remove-pre-push-hook
remove-pre-push-hook:
	[ ! -f .git/hooks/pre-push ] || cp .git/hooks/pre-push .git/hooks/pre-push.backup
	rm .git/hooks/pre-push

install_dependencies_ubuntu::
	$(info === Dependencies for decompressing large files)
	sudo apt install -y \
		zstd

mrproper::
	rm -f $(FILES_TO_DECOMPRESS) $(PATCHELF_PKG)
