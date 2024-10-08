# Copyright 2023-2024 Intel Corporation
# SPDX-License-Identifier: MIT

.PHONY: make_llvm
make_llvm:
	$(info === Build LLLVM)
	+ ./llvm/make_llvm.sh

make_llvm-lldb:
	$(info === Build LLLVM (with LLDB))
	+ CC_LLVM_LLDB=1 ./llvm/make_llvm.sh

make_llvm-lldb-only:
	$(info === Build LLLVM (with LLDB))
	+ CC_LLVM_CONFIGURE_ONLY=1 CC_LLVM_LLDB=1 ./llvm/make_llvm.sh
	+ ninja -C llvm/llvm_build lldb
	+ ninja -C llvm/llvm_build install-lldb
	+ ninja -C llvm/llvm_build install-liblldb

mrproper::
	rm -rf llvm/llvm_build llvm/llvm_install

install_dependencies_ubuntu::
	$(info === glibc dependencies)
	sudo apt install -y \
		cmake \
		ninja-build

.PHONY: llvm
llvm: c3_docker-make_llvm
