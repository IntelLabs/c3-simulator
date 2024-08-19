# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

C3_PYTEST_ARGS := python_tests/test_unit.py --skip-slow
C3_PYTEST_ARGS += --no-upload
C3_PYTEST_ARGS += --durations=0 -rA -s

CKPT_LLVM_TEST_UNIT := checkpoints/cc_llvm-test_unit.ckpt
CKPT_KERNEL_TEST_UNIT := checkpoints/cc_kernel-test_unit.ckpt
CKPT_GLIBC_NOWRAP_TEST_UNIT := checkpoints/cc_glibc-nowrap-test_unit.ckpt

ifneq ($(PYTEST_SLURM),)
	C3_PYTEST_ARGS += -d --slurm
else
	C3_PYTEST_ARGS += -d -n$(shell nproc)
endif

.PHONY: cc_llvm-testing
cc_llvm-testing:
	+ $(MAKE) $(MAKEFLAG_JOBS) UPLOAD_UNIT_TESTS=1 c3_docker-ckpt-cc_llvm CKPT_LLVM=$(CKPT_LLVM_TEST_UNIT)

.PHONY: cc_kernel-testing
cc_kernel-testing:
	+ $(MAKE) $(MAKEFLAG_JOBS) UPLOAD_UNIT_TESTS=1 c3_docker-ckpt-cc_kernel CKPT_KERNEL=$(CKPT_KERNEL_TEST_UNIT)

.PHONY: cc_glibc_nowrap-testing
cc_glibc_nowrap-testing:
	+ $(MAKE) $(MAKEFLAG_JOBS) UPLOAD_UNIT_TESTS=1 c3_docker-ckpt-cc_glibc_nowrap CKPT_GLIBC_NOWRAP=$(CKPT_GLIBC_NOWRAP_TEST_UNIT)

.PHONY: test_unit-build_ckpts
test_unit-build_ckpts:
	# We need to serialize this as parallel checkpoint builds are not supported.
	+ $(MAKE) $(MAKEFLAG_JOBS) cc_llvm-testing
	+ $(MAKE) $(MAKEFLAG_JOBS) cc_kernel-testing
	+ $(MAKE) $(MAKEFLAG_JOBS) cc_glibc_nowrap-testing

.PHONY: test_unit-cc_llvm
test_unit-cc_llvm:
	pytest $(C3_PYTEST_ARGS) --checkpoint $(CKPT_LLVM_TEST_UNIT) --nomodel c3-nowrap --nomodel cc-nowrap

.PHONY: test_unit-cc_kernel
test_unit-cc_kernel:
	pytest $(C3_PYTEST_ARGS) --checkpoint $(CKPT_KERNEL_TEST_UNIT) --have-kernel --nomodel lim --nomodel c3-nowrap --nomodel cc-nowrap

.PHONY: test_unit-cc_glibc_nowrap
test_unit-cc_glibc_nowrap:
	pytest $(C3_PYTEST_ARGS) --checkpoint $(CKPT_GLIBC_NOWRAP_TEST_UNIT) --have-kernel --default-unwinder --model cc-nowrap --model c3-nowrap

.PHONY: cc-test-all
cc-test-all:
	+ $(MAKE) test_unit-cc_llvm
	+ $(MAKE) test_unit-cc_kernel
	+ $(MAKE) test_unit-cc_glibc_nowrap
