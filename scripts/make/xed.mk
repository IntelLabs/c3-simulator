# Copyright 2023 Intel Corporation
# SPDX-License-Identifier: MIT

ifndef SCRIPTS_MAKE_XED_MK_
SCRIPTS_MAKE_XED_MK_ =1

XED_GIT_URL = https://github.com/intelxed/xed.git
XED_GIT_TAG = v2022.10.11
MBUILD_GIT_URL = https://github.com/intelxed/mbuild.git
MBUILD_GIT_TAG = v2022.07.28

XED_INSTALL_PATH = $(project_dir)/lib/xed/kits/xed

XED_MFILE_ARGS = install --extra-flags=-fPIC
XED_MFILE_ARGS += --install-dir=$(XED_INSTALL_PATH)

lib/mbuild:
	mkdir -p $@
	git -C $@ clone -b $(MBUILD_GIT_TAG) --depth 1 $(MBUILD_GIT_URL) .

lib/xed:
	mkdir -p $@
	git -C $@ clone -b $(XED_GIT_TAG) --depth 1 $(XED_GIT_URL) .

$(XED_INSTALL_PATH)/lib/libxed.a: lib/xed lib/mbuild
	cd lib/xed && ./mfile.py $(XED_MFILE_ARGS)

.PHONY: install-xed
install-xed: $(XED_INSTALL_PATH)/lib/libxed.a

mrproper::
	rm -rf $(XED_INSTALL_PATH)

endif
