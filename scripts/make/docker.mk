# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

workdir = /c3_workdir
docker_home = /home/c3_user


# We have two layers of Dockerfiles:
#
# DOCKER_BASE
#    This one has most of the dependencies and tools installed but no user-
#    specific configurations, i.e., it can be reused between users on host.
#
# DOCKER
#    This on build on DOCKER_BASE and configures UID/GUID to match the current
#    user on the host so we can share build directories and sources easily
#    between the container and host
#
DOCKER_BASE_DOCKERFILE = $(project_dir)/scripts/docker/Dockerfile_base
DOCKER_DOCKERFILE = $(project_dir)/scripts/docker/Dockerfile

# Generate a truncated sha256sum over the Dockerfiles so we can use that to
# detect changes and rebuild images only when the checksum changes.
DOCKER_BASE_SHA = $(shell sha256sum $(DOCKER_BASE_DOCKERFILE) | head -c16)
DOCKER_SHA = $(shell sha256sum $(DOCKER_DOCKERFILE) | head -c16)

# Compose the tags / image names form the Dockerfile checksums and also include
# the host username in the user-specific image.
DOCKER_BASE_TAG := c3_base.$(DOCKER_BASE_SHA)
DOCKER_TAG := $(shell whoami)/c3_docker.$(DOCKER_SHA).$(DOCKER_BASE_SHA)

SIMICS_ISPM ?= intel-simics-package-manager-1.8.3
SIMICS_ISPM_PKG = $(SIMICS_ISPM)-linux64.tar.gz
SIMICS_BUNDLE_PKG ?= simics-6-packages-2024-05-linux64.ispm
SIMICS_PUB_URL = https://software.intel.com/content/www/us/en/develop/articles/simics-simulator.html
SIMICS_MISSING_MESSAGE = "Please download Simics installation packages:"
SIMICS_MISSING_MESSAGE += "\\n\\t$(SIMICS_BUNDLE_PKG)\\n\\t$(SIMICS_ISPM_PKG)"
SIMICS_MISSING_MESSAGE += "\\nto scripts/docker."
SIMICS_MISSING_MESSAGE += "\\n\\nPublic Simics release can be found at $(SIMICS_PUB_URL)\\n"

# Base stuff
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/malloc",target="$(workdir)/malloc",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/crypto",target="$(workdir)/crypto",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/scripts",target="$(workdir)/scripts",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/c3lib",target="$(workdir)/c3lib",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/microbenchmarks",target="$(workdir)/microbenchmarks",readonly

# Makefiles
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/config-user.mk",target="$(workdir)/config-user.mk",readonly

# Share the Buildroot ccache folder with the container
ifneq ($(wildcard ${HOME}/.c3-buildroot-ccache),)
DOCKER_ARGS += --mount type=bind,source="${HOME}/.c3-buildroot-ccache",target="$(docker_home)/.c3-buildroot-ccache"
endif
# Share common Buildroot toolchain, it it exists
ifneq ($(wildcard /opt/simics/buildroot_toolchains/*),)
DOCKER_ARGS += --mount type=bind,source="/opt/simics/buildroot_toolchains",target="/opt/simics/buildroot_toolchains",readonly
endif
# Share the LLVM ccache folder if it exists
ifneq ($(wildcard ${HOME}/.c3-llvm-ccache),)
DOCKER_ARGS += --mount type=bind,source="${HOME}/.c3-llvm-ccache",target="$(docker_home)/.c3-llvm-ccache"
endif
# Share the Linux kernel ccache folder if it exists
ifneq ($(wildcard ${HOME}/.c3-linux-ccache),)
DOCKER_ARGS += --mount type=bind,source="${HOME}/.c3-linux-ccache",target="$(docker_home)/.c3-linux-ccache"
endif

# Simics stuff
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/modules",target="$(workdir)/modules",readonly

# Checkpoints
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/checkpoints",target="$(workdir)/checkpoints"

# Linux
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/linux/src",target="$(workdir)/linux/src"
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/linux/configs",target="$(workdir)/linux/configs",readonly

# LLVM
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/llvm/src",target="$(workdir)/llvm/src",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/llvm/make_llvm.sh",target="$(workdir)/llvm/make_llvm.sh",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/llvm/llvm_install",target="$(workdir)/llvm/llvm_install"
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/llvm/llvm_docker_build",target="$(workdir)/llvm/llvm_build"
c3_docker::
	mkdir -p llvm/llvm_docker_build
	mkdir -p llvm/llvm_install

# Glibc
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/glibc/src",target="$(workdir)/glibc/src",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/glibc/make_glibc.sh",target="$(workdir)/glibc/make_glibc.sh",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/glibc/glibc-2.30_install",target="$(workdir)/glibc/glibc-2.30_install"
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/glibc/glibc-2.30_docker_build",target="$(workdir)/glibc/glibc-2.30_build"
c3_docker::
	mkdir -p glibc/glibc-2.30_docker_build
	mkdir -p glibc/glibc-2.30_install

# Shared Simics checkpoints
ifneq ($(wildcard /opt/simics/checkpoints/*),)
ifeq ($(DOCKER_WRITABLE_SHARED_CHECKPOINTS),1)
DOCKER_ARGS += --mount type=bind,source="/opt/simics/checkpoints",target="/opt/simics/checkpoints"
else
DOCKER_ARGS += --mount type=bind,source="/opt/simics/checkpoints",target="/opt/simics/checkpoints",readonly
endif
endif

# Juliet tests
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/tests",target="$(workdir)/tests"

# Unit tests
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/unit_tests",target="$(workdir)/unit_tests",readonly

# Logs and debug folder
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/debug",target="$(workdir)/debug"
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/logs",target="$(workdir)/logs"
c3_docker::
	mkdir -p debug
	mkdir -p logs

# EDK2
ifneq ($(wildcard edk2_src/*),)
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/edk2_src/Makefile",target="$(workdir)/edk2_src/Makefile",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/edk2_src/configs",target="$(workdir)/edk2_src/configs",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/edk2_src/scripts",target="$(workdir)/edk2_src/scripts",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/edk2_src/Build",target="$(workdir)/edk2_src/Build"
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/edk2_src/FSP",target="$(workdir)/edk2_src/FSP"
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/edk2_src/edk2",target="$(workdir)/edk2_src/edk2"
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/edk2_src/edk2_buildroot",target="$(workdir)/edk2_src/edk2_buildroot"
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/edk2_src/edk2-non-osi",target="$(workdir)/edk2_src/edk2-non-osi"
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/edk2_src/edk2-platforms",target="$(workdir)/edk2_src/edk2-platforms"
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/buildroot",target="$(workdir)/buildroot"
c3_docker::
	mkdir -p edk2_src/Build
	mkdir -p edk2_src/configs
	mkdir -p edk2_src/edk2_buildroot
	mkdir -p buildroot
endif


ifdef INTERACTIVE
DOCKER_ARGS += -it
endif

# X11 Forwarding (doesn't seem to work)
DOCKER_ARGS += -e DISPLAY=${DISPLAY} -v /tmp/.X11-unix/:/tmp/.X11-unix/
DOCKER_ARGS += -v "/home/$(shell whoami)/.Xauthority:/home/c3_user/.Xauthority:rw"

scripts/docker/c3_docker.key:
	ssh-keygen -t rsa -N "" -C $(DOCKER_TAG) -f $@


# Make sure we have the $(SIMICS_BUNDLE_PKG) installation files. If available
# in a shared /opt/simics/simics_packages_public, then copy from there.
scripts/docker/$(SIMICS_BUNDLE_PKG):
ifeq (,$(wildcard /opt/simics/simics_packages_public/$(SIMICS_BUNDLE_PKG)))
	@echo "Cannot find scripts/docker/$(SIMICS_BUNDLE_PKG)!\\n"
	@echo "$(SIMICS_MISSING_MESSAGE)"
	false
else
	cp /opt/simics/simics_packages_public/$(SIMICS_BUNDLE_PKG) $@
endif

# Make sure we have the $(SIMICS_BUNDLE_PKG) installation files. If available
# in a shared /opt/simics/simics_packages_public, then copy from there.
scripts/docker/$(SIMICS_ISPM_PKG):
ifeq (,$(wildcard /opt/simics/simics_packages_public/$(SIMICS_ISPM_PKG)))
	@echo "Cannot find scripts/docker/$(SIMICS_ISPM_PKG)!\\n"
	@echo "$(SIMICS_MISSING_MESSAGE)"
	false
else
	cp /opt/simics/simics_packages_public/$(SIMICS_ISPM_PKG) $@
endif

# The target for the base docker image.
#
# If it doesn't exist, create a $(DOCKER_BASE_TAG) Docker image. The name of the
# image consists of a checksum over docker configuration files, and will be re-
# created only if not previously created or if configuration (and hence the tag)
# is changed.
PHONY: $(DOCKER_BASE_TAG)
$(DOCKER_BASE_TAG): scripts/docker/$(SIMICS_BUNDLE_PKG) scripts/docker/$(SIMICS_ISPM_PKG)
	mkdir -p checkpoints
ifeq (,$(shell command -v docker >/dev/null 2>&1 && \
               docker image ls | grep $(DOCKER_BASE_TAG)))
	docker build scripts/docker -t $(DOCKER_BASE_TAG) \
		-f $(DOCKER_BASE_DOCKERFILE) \
		--build-arg SIMICS_ISPM="$(SIMICS_ISPM)" \
		--build-arg SIMICS_BUNDLE_PKG="$(SIMICS_BUNDLE_PKG)"
endif

# Target for the user-specific c3_docker image
#
# Actual image name will be USERNAME/c3_docker.DOCKERFFILE_SHA.
#
.PHONY: c3_docker
c3_docker:: scripts/docker/c3_docker.key $(DOCKER_BASE_TAG)
	mkdir -p checkpoints
ifeq (,$(shell command -v docker >/dev/null 2>&1 && \
               docker image ls | grep $(DOCKER_TAG)))
	docker build scripts/docker -t $(DOCKER_TAG)   \
		--build-arg C3_DOCKER_BASE_TAG=$(DOCKER_BASE_TAG) \
		--build-arg C3_USER_UID=$(shell id -u) \
		--build-arg C3_USER_GID=$(shell id -g)
endif

.PHONY: c3_docker_shell
c3_docker_shell: c3_docker
	{ \
		xhost +; \
		docker run -u c3_user -h $(shell whoami) --rm -it $(DOCKER_ARGS) \
			$(DOCKER_TAG) zsh; \
	}

.PHONY: c3_docker_start
c3_docker_start: c3_docker
	echo
	echo "Launching c3_docker as $(DOCKER_TAG)"
	echo
	echo "To SSH, use one of:"
	echo "   ssh -p 2222 c3_user@localhost (password: c3_user)"
	echo "   make c3_docker_ssh"
	echo
	echo "To stop container:"
	echo "   docker stop $(DOCKER_TAG)"
	echo
	docker run -d --rm -it $(DOCKER_ARGS) -p 2222:22 \
		--name $(shell whoami).$(DOCKER_TAG) $(DOCKER_TAG); \

.PHONY: c3_docker_stop
c3_docker_stop:
	docker stop $(shell whoami).$(DOCKER_TAG)

.PHONY: c3_docker_ssh
c3_docker_ssh:
	ssh-keygen -f "${HOME}/.ssh/known_hosts" -R "[localhost]:2222"
	ssh -p 2222 -i scripts/docker/c3_docker.key \
	    -o StrictHostKeyChecking=no             \
	    -X c3_user@localhost -v

.PHONY: c3_docker_ubuntu
c3_docker_ubuntu: c3_docker-ckpt-ubuntu

EXTRA_DOCKER_MAKE_ARGS += $(MAKEFLAG_JOBS)
EXTRA_DOCKER_MAKE_ARGS += $(MAKEFLAG_N)

ifeq (${UPLOAD_UNIT_TESTS},1)
	EXTRA_DOCKER_MAKE_ARGS += "UPLOAD_UNIT_TESTS=1"
endif

ifeq (${UPLOAD_SPEC_TESTS},1)
	EXTRA_DOCKER_MAKE_ARGS += "UPLOAD_SPEC_TESTS=1"
endif

ifeq (${VERBOSE},1)
	EXTRA_DOCKER_MAKE_ARGS += "VERBOSE=1"
endif

EXTRA_DOCKER_MAKE_ARGS += CKPT_LLVM=$(CKPT_LLVM)
EXTRA_DOCKER_MAKE_ARGS += CKPT_KERNEL=$(CKPT_KERNEL)
EXTRA_DOCKER_MAKE_ARGS += CKPT_GLIBC_NOWRAP=$(CKPT_GLIBC_NOWRAP)
EXTRA_DOCKER_MAKE_ARGS += CKPT_KERNEL_BASE=$(CKPT_KERNEL_BASE)
EXTRA_DOCKER_MAKE_ARGS += "VERSION_LABEL=$(VERSION_LABEL)_docker"

DOCKER_RUN_ARGS = -u c3_user -h $(shell whoami) --rm $(DOCKER_ARGS) $(DOCKER_TAG)

c3_docker-%:: c3_docker
ifeq ($(NO_DOCKER),)
	docker run $(DOCKER_RUN_ARGS) make $* $(EXTRA_DOCKER_MAKE_ARGS)
else
	+ $(MAKE) $(MAKEFLAG_N) $*
endif
