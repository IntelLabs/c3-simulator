# Label for checkpoints; this will be postifxed to checkpoint names and the
# non-labeled name will be symlinked to the labeled one.
ckpt_tag ::= $(shell git rev-parse --short HEAD)

# Target and folder names for the checkpoints
#
# The actual folder will be postfixed with a timestamp, and the non-checkpointed
# name will be a symlink to the timestamped checkpoint. These are dump targets,
# i.e., they will always be re- created when executed, but only the symlink will
# be deleted, old timestamped checkpoints will remain untouched.
CKPT_GLIBC      ?= checkpoints/cc_glibc.ckpt
CKPT_LLVM       ?= checkpoints/cc_llvm.ckpt
CKPT_KERNEL     ?= checkpoints/cc_kernel.ckpt
CKPT_DEBUGGER   ?= checkpoints/debugger.ckpt

# Initial base checkpoints to build upon
#
# These are expected to be in a state that our Simics scirpts can run as-is,
# e.g., the simics agent is epxected to be already running on the guest and
# ready to connect.
CKPT_NOKERNEL_BASE ?= $(wildcard /opt/simics/checkpoints/glibc_latest.ckpt)
CKPT_KERNEL_BASE ?= $(wildcard /opt/simics/checkpoints/ubuntu-20.4_latest.ckpt)

CHECKPOINT_TARGETS = $(CKPT_KERNEL) $(CKPT_LLVM) $(CKPT_GLIBC) $(CKPT_DEBUGGER)
CHECKPOINT_TARGETS += $(CKPT_NOKERNEL_BASE)

SIMICS_NOKERNEL_CHECKPOINT_ARG =
ifneq ($(CKPT_NOKERNEL_BASE),)
	SIMICS_NOKERNEL_CHECKPOINT_ARG = checkpoint=$(CKPT_NOKERNEL_BASE)
endif

# Target for creating new shared checkpoint
$(CKPT_NOKERNEL_BASE).$(ckpt_tag): simics_setup make_llvm make_glibc-shim
	$(info === Creating Simics checkpoint $@ (glibc, libunwind))
	./simics -batch-mode scripts/update_libs.simics \
		save_checkpoint=$@

# Target for creating local no-kernel checkpoint
$(CKPT_GLIBC).$(ckpt_tag): simics_setup make_glibc-shim
	$(info === Creating Simics checkpoint $@ (glibc))
	./simics -batch-mode scripts/update_libs.simics \
		$(SIMICS_NOKERNEL_CHECKPOINT_ARG) \
		do_glibc=TRUE \
		do_llvm=FALSE \
		save_checkpoint=$@

$(CKPT_LLVM).$(ckpt_tag): simics_setup make_llvm make_glibc-shim
	$(info === Creating Simics checkpoint $@ (glibc, libunwind))
	./simics -batch-mode scripts/update_libs.simics \
		$(SIMICS_NOKERNEL_CHECKPOINT_ARG) \
		do_glibc=TRUE \
		do_llvm=TRUE \
		save_checkpoint=$@

# Target for creating local custom-kernel checkpoint
$(CKPT_KERNEL).$(ckpt_tag): simics_setup make_llvm make_glibc-noshim linux/linux.tar.gz
	$(info === Creating Simics checkpoint $@ (glibc, libunwind, linux))
	./simics -batch-mode scripts/update_ubuntu_kernel.simics \
		checkpoint=$(CKPT_KERNEL_BASE) \
		upload_llvm=TRUE \
		upload_glibc=TRUE \
		kernel=linux/linux.tar.gz \
		save_checkpoint=$@

# Target with in-guest built kernel (NOTE: very slow to build!)
$(CKPT_DEBUGGER).$(ckpt_tag): simics_setup make_llvm
	$(info === Creating Simics checkpoint $@ (glibc, libunwind, lldb, linux)
	./simics -batch-mode scripts/update_libs.simics \
		do_llvm=TRUE \
		do_glibc=TRUE \
		llvm_buildmode=upload \
		glibc_buildmode=build \
		checkpoint=$(CKPT_KERNEL) \
		save_checkpoint=$@

.PHONY: $(CHECKPOINT_TARGETS)
$(CHECKPOINT_TARGETS): % : %.$(ckpt_tag)
	$(info === Linking $@ -> $@.$(ckpt_tag))
	rm -f $@
	cd $(dir $@) && ln -s $(notdir $@).$(ckpt_tag) $(notdir $@)

.PHONY: ckpt-cc_glibc
ckpt-cc_glibc: $(CKPT_GLIBC)

.PHONY: ckpt-cc_llvm
ckpt-cc_llvm: $(CKPT_LLVM)

.PHONY: ckpt-cc_kernel
ckpt-cc_kernel: $(CKPT_KERNEL)

.PHONY: update-base-ckpts
update-base-ckpts: $(CKPT_NOKERNEL_BASE)

.PHONY: clean-checkpoints
clean-checkpoints:

mrproper:: clean-checkpoints
	rm -f $(CKPT_GLIBC) $(CKPT_LLVM) $(CKPT_KERNEL) $(CKPT_DEBUGGER)
