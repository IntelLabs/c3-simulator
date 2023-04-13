
.PHONY: make_glibc-noshim
make_glibc-noshim:
	$(info === Build glibc noshim)
	CC_USE_SYSCALL_SHIMS=0 ./glibc/make_glibc.sh

.PHONY: make_glibc-shim
make_glibc-shim:
	$(info === Build glibc shim)
	CC_USE_SYSCALL_SHIMS=1 ./glibc/make_glibc.sh

mrproper::
	rm -rf glibc/glibc-2.30_build glibc/glibc-2.30_install

install_dependencies_ubuntu::
	$(info === glibc dependencies)
	sudo apt install -y \
		gawk
