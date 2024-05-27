
.PHONY: glibc-noshim
glibc-noshim:
	$(info === Build $@)
	+ CC_USE_SYSCALL_SHIMS=0 ./glibc/make_glibc.sh

.PHONY: glibc-shim
glibc-shim:
	$(info === Build $@)
	+ CC_USE_SYSCALL_SHIMS=1 ./glibc/make_glibc.sh

.PHONY: glibc-nowrap
glibc-nowrap:
	$(info === Build $@)
	+ CC_NO_WRAP_ENABLE=1 ./glibc/make_glibc.sh

.PHONY: make_glibc-noshim
make_glibc-noshim: glibc-noshim

.PHONY: make_glibc-shim
make_glibc-shim: glibc-shim

.PHONY: make_glibc-nowrap
make_glibc-nowrap: glibc-nowrap

mrproper::
	rm -rf glibc/glibc-2.30_build glibc/glibc-2.30_install

install_dependencies_ubuntu::
	$(info === glibc dependencies)
	sudo apt install -y \
		gawk
