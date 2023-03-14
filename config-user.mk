# Generate absolute path in case we need it elsewher
project_dir := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Local / user config file that can be used to override some variables.
-include config-local.mk
# Some possible variables you may want to override are listed below. See
# individual makefile scripts for default values and current configuration as
# these examples may not be up to date and are just for refrence.
#
# scripts/make/checkpoints.mk
# 	CKPT_NOKERNEL_BASE
# 	CKPT_KERNEL_BASE
# 	CKPT_NOKERNEL
# 	CKPT_KERNEL
#
# scripts/make/init_and_setup.mk
# 	SIMICS_BIN 	       	- Path to Simics installation bin directory
#

# Project makefiles
include scripts/make/checkpoints.mk
include scripts/make/compile_commands.mk
include scripts/make/dependencies.mk
include scripts/make/documentation.mk
include scripts/make/glibc.mk
include scripts/make/init_and_setup.mk
include scripts/make/linux.mk
include scripts/make/llvm.mk
include scripts/make/pre-commit.mk

# Use `make mrproper` to clean up build artifacts and other files.
#
# Separate makefiles can define their own mrproper:: targets (with two colons).
# For instance, the scripts/make/llvm.mk has its own mrproper target that cleans
# LLVM temporaries. Running `make mrproper` will excecute all mrproper targets
# accross the makefiles.
.PHONY: mrproper
mrproper:: clean

# Allows print makefile variables, e.g., print-SIMICS_BIN
print-%:
	@echo $* = $($*)
