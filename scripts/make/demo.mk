# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

PHONY: demo-lldb_debug_01
demo-lldb_debug_01:
	test ! -e checkpoints/cc_kernel.ckpt || $(MAKE) ckpt-cc_kernel
	$(MAKE) make_llvm-lldb-only
ifeq (1,$(VERBOSE))
	./scripts/demo/lldb_debug_01.sh --verbose
else
	./scripts/demo/lldb_debug_01.sh
endif


.PHONY: scripts/demo/cwe457.sh
scripts/demo/cwe457.sh:
	test ! -e checkpoints/cc_kernel.ckpt || $(MAKE) ckpt-cc_kernel
	$(MAKE) make_llvm

.PHONY: scripts/demo/clang_tidy.sh
scripts/demo/clang_tidy.sh:
	test ! -e checkpoints/cc_kernel.ckpt || $(MAKE) ckpt-cc_kernel
	$(MAKE) make_llvm

demo-%: scripts/demo/%
	$(MAKE)
	./scripts/demo/$*

demo-juliet: tests/nist-juliet/scripts/run_juliet.sh
	$(MAKE)
	./tests/nist-juliet/scripts/run_juliet.sh

demo-juliet-native: tests/nist-juliet/scripts/run_juliet.sh
	$(MAKE)
	./tests/nist-juliet/scripts/run_juliet.sh -m native

demo-juliet-native-stack: tests/nist-juliet/scripts/run_juliet.sh
	$(MAKE)
	./tests/nist-juliet/scripts/run_juliet.sh -m native -s 1

demo-juliet-c3-heap: tests/nist-juliet/scripts/run_juliet.sh
	$(MAKE)
	./tests/nist-juliet/scripts/run_juliet.sh -m c3 -h 1

demo-juliet-c3-heap-align: tests/nist-juliet/scripts/run_juliet.sh
	$(MAKE)
	./tests/nist-juliet/scripts/run_juliet.sh -m c3 -h 1 -a 1

demo-juliet-c3-stack: tests/nist-juliet/scripts/run_juliet.sh
	$(MAKE)
	./tests/nist-juliet/scripts/run_juliet.sh -m c3 -s 1
