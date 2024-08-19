# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

# Label for checkpoints; this will be postfixed to checkpoint names and the
# non-labeled name will be symlinked to the labeled one.
# (e.g., `cc_llvm.ckpt -> cc_llvm.$(VERSION_LABEL).ckpt`)
ifeq ($(VERSION_LABEL),)
VERSION_LABEL ::= $(shell git rev-parse --short HEAD 2> /dev/null)
endif
ifeq ($(VERSION_LABEL),)
VERSION_LABEL ::= nogit
endif

# Target and folder names for the checkpoints
#
# The actual folder will be postfixed with a timestamp, and the non-checkpointed
# name will be a symlink to the timestamped checkpoint. These are dump targets,
# i.e., they will always be re- created when executed, but only the symlink will
# be deleted, old timestamped checkpoints will remain untouched.
CKPT_GLIBC      	?= checkpoints/cc_glibc.ckpt
CKPT_GLIBC_NOWRAP	?= checkpoints/cc_glibc_nowrap.ckpt
CKPT_LLVM       	?= checkpoints/cc_llvm.ckpt
CKPT_KERNEL     	?= checkpoints/cc_kernel.ckpt
CKPT_KERNEL_1B_OVF 	?= checkpoints/cc_kernel_1b_ovf.ckpt
CKPT_KERNEL_PROT	?= checkpoints/cc_kernel_prot.ckpt
CKPT_DEBUGGER   	?= checkpoints/cc_kernel_lldb.ckpt

# Initial base checkpoints to build upon
#
# These are expected to be in a state that our Simics scripts can run as-is,
# e.g., the simics agent is expected to be already running on the guest and
# ready to connect.
CKPT_NOKERNEL_BASE ?= $(wildcard /opt/simics/checkpoints/glibc_latest.ckpt)
ifeq (,$(CKPT_KERNEL_BASE))
CKPT_KERNEL_BASE := $(wildcard checkpoints/ubuntu_base.ckpt)
endif
ifeq (,$(CKPT_KERNEL_BASE))
CKPT_KERNEL_BASE := $(wildcard /opt/simics/checkpoints/ubuntu-20.4_latest.ckpt)
endif
ifeq (,$(CKPT_KERNEL_BASE))
CKPT_KERNEL_BASE := checkpoints/ubuntu_base.ckpt
endif

SIMICS_KERNEL_CHECKPOINT_ARG = checkpoint=$(shell readlink -e $(CKPT_KERNEL_BASE))

CHECKPOINT_TARGETS = $(CKPT_KERNEL) $(CKPT_KERNEL_1B_OVF) $(CKPT_LLVM) $(CKPT_GLIBC) $(CKPT_DEBUGGER)
CHECKPOINT_TARGETS += $(CKPT_NOKERNEL_BASE) $(CKPT_KERNEL_PROT)
CHECKPOINT_TARGETS += $(CKPT_KERNEL_BASE) $(CKPT_GLIBC_NOWRAP)

CHECKPOINTS_TO_CLEAN = $(CKPT_KERNEL) $(CKPT_KERNEL_1B_OVF) $(CKPT_LLVM) $(CKPT_GLIBC)
CHECKPOINTS_TO_CLEAN += $(CKPT_DEBUGGER) $(CKPT_KERNEL_PROT)
CHECKPOINTS_TO_CLEAN += $(CKPT_GLIBC_NOWRAP)


SIMICS_NOKERNEL_CHECKPOINT_ARG =
ifneq ($(CKPT_NOKERNEL_BASE),)
	SIMICS_NOKERNEL_CHECKPOINT_ARG = checkpoint=$(shell readlink -e $(CKPT_NOKERNEL_BASE))
endif

CKPT_SIMICS_ARGS = -batch-mode

CKPT_SIMICS_SCRIPT_ARGS =

ifeq (${UPLOAD_UNIT_TESTS},1)
	CKPT_SIMICS_SCRIPT_ARGS += upload_unit_tests=TRUE
endif


# Target for creating new shared clean ubuntu checkpoint
$(CKPT_KERNEL_BASE).$(VERSION_LABEL): simics_setup
	$(info === Creating Simics checkpoint $@ (Ubuntu clean install))
	test ! -e $@ || (echo "Already exists: $@" && false)
	./simics $(CKPT_SIMICS_ARGS) scripts/install_ubuntu.simics \
		save_checkpoint=$@

# Target for creating new shared checkpoint
$(CKPT_NOKERNEL_BASE).$(VERSION_LABEL): simics_setup make_llvm make_glibc-shim
	$(info === Creating Simics checkpoint $@ (glibc, libunwind))
	test ! -e $@ || (echo "Already exists: $@" && false)
	./simics $(CKPT_SIMICS_ARGS) scripts/update_libs.simics \
		save_checkpoint=$@

# Target for creating local no-kernel checkpoint
$(CKPT_GLIBC).$(VERSION_LABEL): simics_setup make_glibc-shim
	$(info === Creating Simics checkpoint $@ (glibc))
	test ! -e $@ || (echo "Already exists: $@" && false)
	./simics $(CKPT_SIMICS_ARGS) scripts/update_libs.simics \
		$(SIMICS_NOKERNEL_CHECKPOINT_ARG) \
		$(CKPT_SIMICS_SCRIPT_ARGS) \
		do_glibc=TRUE \
		do_llvm=FALSE \
		save_checkpoint=$@


# Target for creating local no-kernel checkpoint
$(CKPT_GLIBC_NOWRAP).$(VERSION_LABEL): simics_setup make_glibc-nowrap make_linux
	$(info === Creating Simics checkpoint $@ (glibc-nowrap, linux))
	test ! -e $@ || (echo "Already exists: $@" && false)
	./simics $(CKPT_SIMICS_ARGS) scripts/update_ubuntu_kernel.simics \
		checkpoint=$(shell readlink -e $(CKPT_KERNEL_BASE)) \
		$(CKPT_SIMICS_SCRIPT_ARGS) \
		upload_llvm=FALSE \
		upload_glibc=TRUE \
		save_checkpoint=$@

$(CKPT_LLVM).$(VERSION_LABEL): simics_setup make_llvm make_glibc-shim
	$(info === Creating Simics checkpoint $@ (glibc, libunwind))
	test ! -e $@ || (echo "Already exists: $@" && false)
	./simics $(CKPT_SIMICS_ARGS) scripts/update_libs.simics \
		$(SIMICS_NOKERNEL_CHECKPOINT_ARG) \
		$(CKPT_SIMICS_SCRIPT_ARGS) \
		do_glibc=TRUE \
		do_llvm=TRUE \
		save_checkpoint=$@

# Target for creating local custom-kernel checkpoint
$(CKPT_KERNEL).$(VERSION_LABEL): simics_setup make_llvm make_glibc-noshim make_linux
	$(info === Creating Simics checkpoint $@ (glibc, libunwind, linux))
	test ! -e $@ || (echo "Already exists: $@" && false)
	./simics $(CKPT_SIMICS_ARGS) scripts/update_ubuntu_kernel.simics \
		$(SIMICS_KERNEL_CHECKPOINT_ARG) \
		$(CKPT_SIMICS_SCRIPT_ARGS) \
		upload_llvm=TRUE \
		upload_glibc=TRUE \
		save_checkpoint=$@

$(CKPT_KERNEL_1B_OVF).$(VERSION_LABEL): simics_setup make_llvm make_glibc-noshim-detect-1b-ovf make_linux
	$(info === Creating Simics checkpoint $@ (glibc, libunwind, linux))
	test ! -e $@ || (echo "Already exists: $@" && false)
	./simics $(CKPT_SIMICS_ARGS) scripts/update_ubuntu_kernel.simics \
		$(SIMICS_KERNEL_CHECKPOINT_ARG) \
		$(CKPT_SIMICS_SCRIPT_ARGS) \
		upload_llvm=TRUE \
		upload_glibc=TRUE \
		save_checkpoint=$@

# Target for creating local custom-kernel checkpoint
$(CKPT_KERNEL_PROT).$(VERSION_LABEL): simics_setup make_linux
	$(info === Creating Simics checkpoint $@ (glibc, libunwind, linux))
	test ! -e $@ || (echo "Already exists: $@" && false)
	./simics $(CKPT_SIMICS_ARGS) scripts/update_ubuntu_kernel.simics \
		$(SIMICS_KERNEL_CHECKPOINT_ARG) \
		$(CKPT_SIMICS_SCRIPT_ARGS) \
		upload_llvm=FALSE \
		upload_glibc=FALSE \
		save_checkpoint=$@

# Target with in-guest built kernel (NOTE: very slow to build!)
$(CKPT_DEBUGGER).$(VERSION_LABEL): simics_setup make_llvm-lldb make_glibc-noshim $(CKPT_KERNEL).$(VERSION_LABEL)
	$(info === Creating Simics checkpoint $@ (glibc, libunwind, lldb, linux))
	test ! -e $@ || (echo "Already exists: $@" && false)
	./simics $(CKPT_SIMICS_ARGS) scripts/update_libs.simics \
		checkpoint=$(shell readlink -e $(CKPT_KERNEL).$(VERSION_LABEL)) \
		$(CKPT_SIMICS_SCRIPT_ARGS) \
		do_llvm=TRUE \
		do_glibc=TRUE \
		llvm_buildmode=upload \
		glibc_buildmode=build \
		save_checkpoint=$@

.PHONY: $(CHECKPOINT_TARGETS)
$(CHECKPOINT_TARGETS): % : %.$(VERSION_LABEL)
	$(info === Linking $@ -> $@.$(VERSION_LABEL))
	rm -f $@
	cd $(dir $@) && ln -s $(notdir $@).$(VERSION_LABEL) $(notdir $@)

.PHONY: ckpt-cc_glibc
ckpt-cc_glibc: $(CKPT_GLIBC)

.PHONY: ckpt-cc_glibc_nowrap
ckpt-cc_glibc_nowrap: $(CKPT_GLIBC_NOWRAP)

.PHONY: ckpt-cc_llvm
ckpt-cc_llvm: $(CKPT_LLVM)

.PHONY: ckpt-cc_kernel
ckpt-cc_kernel: $(CKPT_KERNEL)

.PHONY: ckpt-cc_kernel_1b_ovf
ckpt-cc_kernel_1b_ovf: $(CKPT_KERNEL_1B_OVF)

.PHONY: ckpt-cc_kernel_prot
ckpt-cc_kernel_prot: $(CKPT_KERNEL_PROT)

.PHONY: update-base-ckpts
update-base-ckpts: $(CKPT_NOKERNEL_BASE) $(CKPT_KERNEL_BASE)

.PHONY: ckpt-cc_kernel_lldb
ckpt-cc_kernel_lldb: $(CKPT_DEBUGGER)

.PHONY: ckpt-ubuntu
ckpt-ubuntu: $(CKPT_KERNEL_BASE)

.PHONY: cc_glibc
cc_glibc: c3_docker-ckpt-cc_glibc

.PHONY: cc_llvm
cc_llvm: c3_docker-ckpt-cc_llvm

.PHONY: cc_kernel
cc_kernel: c3_docker-ckpt-cc_kernel

.PHONY: cc_kernel_1b_ovf
cc_kernel_1b_ovf: c3_docker-ckpt-cc_kernel_1b_ovf

.PHONY: cc_glibc_vtag
cc_glibc_vtag: c3_docker-ckpt-cc_glibc_vtag

.PHONY: clean-checkpoints
clean-checkpoints:
	rm -f $(CHECKPOINTS_TO_CLEAN)
	rm -rf $(addsuffix .$(VERSION_LABEL),$(CHECKPOINTS_TO_CLEAN))

have_ubuntu_base_checkpoint:
	echo "$(CKPT_KERNEL_BASE)"
	@test -e $(CKPT_KERNEL_BASE)


.PHONY: mrproper
mrproper:: clean-checkpoints
