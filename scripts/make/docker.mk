
workdir = /c3_workdir

SIMICS_ISPM = intel-simics-package-manager-1.7.5
SIMICS_ISPM_PKG = $(SIMICS_ISPM)-linux64.tar.gz
SIMICS_BUNDLE_PKG = simics-6-packages-2023-31-linux64.ispm
SIMICS_PUB_URL = https://software.intel.com/content/www/us/en/develop/articles/simics-simulator.html
SIMICS_MISSING_MESSAGE = "Please download Simics installation packages:"
SIMICS_MISSING_MESSAGE += "\\n\\t$(SIMICS_BUNDLE_PKG)\\n\\t$(SIMICS_ISPM_PKG)"
SIMICS_MISSING_MESSAGE += "\\nto scripts/docker."
SIMICS_MISSING_MESSAGE += "\\n\\nPublic Simics release can be found at $(SIMICS_PUB_URL)\\n"

# Base stuff
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/malloc",target="$(workdir)/malloc",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/crypto",target="$(workdir)/crypto",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/scripts",target="$(workdir)/scripts",readonly
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/microbenchmarks",target="$(workdir)/microbenchmarks",readonly

# Makefiles
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/config-user.mk",target="$(workdir)/config-user.mk",readonly

# Simics stuff
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/modules",target="$(workdir)/modules",readonly

# Checkpoints
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/checkpoints",target="$(workdir)/checkpoints"

# Linux
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/linux/src",target="$(workdir)/linux/src"
DOCKER_ARGS += --mount type=bind,source="$(project_dir)/linux/default_config",target="$(workdir)/linux/default_config",readonly

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
DOCKER_ARGS += --mount type=bind,source="/opt/simics/checkpoints",target="/opt/simics/checkpoints",readonly
endif




# X11 Forwarding (doesn't seem to work)
DOCKER_ARGS += -e DISPLAY=${DISPLAY} -v /tmp/.X11-unix/:/tmp/.X11-unix/
DOCKER_ARGS += -v "/home/$(shell whoami)/.Xauthority:/home/c3_user/.Xauthority:rw"

scripts/docker/c3_docker.key:
	ssh-keygen -t rsa -N "" -C c3_docker -f $@


.PHONY: c3_docker
c3_docker:: scripts/docker/c3_docker.key
ifeq (,$(wildcard scripts/docker/$(SIMICS_BUNDLE_PKG)))
	@echo "Cannot find scripts/docker/$(SIMICS_BUNDLE_PKG)!\\n"
	@echo "$(SIMICS_MISSING_MESSAGE)"
	false
endif
ifeq (,$(wildcard scripts/docker/$(SIMICS_ISPM_PKG)))
	@echo "Cannot find scripts/docker/$(SIMICS_ISPM_PKG)!\\n"
	@echo "$(SIMICS_MISSING_MESSAGE)"
	false
endif
	mkdir -p checkpoints
	docker build scripts/docker -t c3_docker   \
		--build-arg C3_USER_UID=$(shell id -u) \
		--build-arg C3_USER_GID=$(shell id -g)

.PHONY: c3_docker_shell
c3_docker_shell: c3_docker
	{ \
		xhost +; \
		docker run -u c3_user -h $(shell whoami) --rm -it $(DOCKER_ARGS) c3_docker zsh; \
	}

.PHONY: c3_docker_start
c3_docker_start: c3_docker
	echo
	echo "Launching c3_docker as $(shell whoami).c3_docker"
	echo
	echo "To SSH, use one of:"
	echo "   ssh -p 2222 c3_user@localhost (password: c3_user)"
	echo "   make c3_docker_ssh"
	echo
	echo "To stop container:"
	echo "   docker stop $(shell whoami).c3_docker"
	echo
	docker run -d --rm -it $(DOCKER_ARGS) -p 2222:22 \
		--name $(shell whoami).c3_docker c3_docker; \

.PHONY: c3_docker_stop
c3_docker_stop:
	docker stop $(shell whoami).c3_docker

.PHONY: c3_docker_ssh
c3_docker_ssh:
	ssh -p 2222 -i scripts/docker/c3_docker.key \
	    -o StrictHostKeyChecking=no             \
	    -X c3_user@localhost -v

# Ugly hack to get number of jobs
MAKE_PID := $(shell echo $$PPID)
JOBS := $(shell ps T | \
          sed -n 's%.*$(MAKE_PID).*$(MAKE).* \(-j\|--jobs=\) *\([0-9][0-9]*\).*%\2%p' | \
		  { grep . || echo 1; })

c3_docker-%:: c3_docker
	docker run -u c3_user -h $(shell whoami) --rm -it $(DOCKER_ARGS) c3_docker \
		make $* -j$(JOBS)
