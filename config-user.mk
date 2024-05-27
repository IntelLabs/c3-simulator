# Generate absolute path in case we need it elsewher
project_dir := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

# INTERACTIVE is defined only if running in intractive shell
INTERACTIVE := $(shell [ -t 0 ] && echo 1)
INSIDE_DOCKER := $(shell if [ -f "/proc/1/cgroup" ] && grep -q '/docker/' /proc/1/cgroup; then echo true; fi)

MAKE_PID := $(shell echo $$PPID)
MAKE_JOBS := $(shell ps T | \
          sed -n 's%.*$(MAKE_PID).*$(MAKE).* \(-j\|--jobs=\) *\([0-9][0-9]*\).*%\2%p' | \
		  { grep . || echo 1; })

# MAKEFLAG_JOBS is set if current make had the -j* flag (other than -j1)
ifneq (1,$(MAKE_JOBS))
	MAKEFLAG_JOBS = -j$(MAKE_JOBS)
endif
# MAKEFLAG_N is set if current make had -n flag
ifeq (n,$(findstring n,$(firstword $(MAKEFLAGS))))
	MAKEFLAG_N = -n
endif

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
# Include only if running in project root (by attemptlying ton include the
# files and suppressing errors with - if the relative path doesn't find the
# files, i.e., we're not in the root)
-include scripts/make/checkpoints.mk
-include scripts/make/cmake.mk
-include scripts/make/compile_commands.mk
-include scripts/make/demo.mk
-include scripts/make/dependencies.mk
-include scripts/make/docker.mk
-include scripts/make/documentation.mk
-include scripts/make/edk2.mk
-include scripts/make/glibc.mk
-include scripts/make/init_and_setup.mk
-include scripts/make/linux.mk
-include scripts/make/llvm.mk
-include scripts/make/pre-commit.mk

# ENV vars added by Simics Makefiles (i.e., used by the Simics build)
SIMICS_ENV_FLAGS = _RAW_EML_PACKAGE _SHELLQUOTE SIMICS_WORKSPACE DMLC INCLUDE_PATHS DMLC_DIR USER_BUILD_ID CPP PY2TO3 D MAKELEVEL CXX SIMICS_MODEL_BUILDER CCLD DEP_CFLAGS _MAKEQUOTE LIBS PYTHON MAKE_TERMERR MAKEFLAGS PYTHON3_LDFLAGS DEP_CXX COMPILER OBJEXT PYTHON_INCLUDE DISAS _RAW_SIMICS_BASE PYTHON_BLD_CFLAGS PYTHON3_INCLUDE CC_TYPE LDFLAGS_PY3 DMLC_OPT_FLAGS _SYSTEMC_DML_PACKAGE SIMICS_MAJOR_VERSION MODULE_MAKEFILE CXX_INCLUDE_PATHS MODULE_DIRS GCC_VISIBILITY CCLDFLAGS_DYN BLD_CFLAGS DEP_CXXFLAGS DEP_DOUBLE_TARGETS BLD_CXXFLAGS CXXFLAGS SIMICS_PACKAGE_LIST PYTHON_LDFLAGS SIMICS_PROJECT SO_SFX BLD_OPT_CFLAGS TARGET_DIR DML_INCLUDE_PATHS V DEP_CC CAT CC LDFLAGS AR LIBZ PYTHON3 CFLAGS SIMICS_BASE MKDIRS MFLAGS _IS_WINDOWS
# Environment variables to keep when launching clean environments (e.g., to
# lauch targets in Makefiles not included directly above).
SUB_ENV_FLAGS= $(addprefix -u ,$(SIMICS_ENV_FLAGS))

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
