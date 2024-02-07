
install_dependencies_ubuntu::
	$(info === Depencencies for Linux builds)
	sudo apt install -y \
		build-essential \
		bison \
		dwarves \
		flex \
		libelf-dev \
		libssl-dev \
		llvm \
		zstd

.PHONY: make_linux
make_linux: linux/src/.config
	$(info === Build Linux kernel)
	make -C linux/src -j$(shell nproc)

linux/src/.config: linux/default_config
	cp -f $< $@

linux-config: linux/src/.config
	make -C linux/src oldconfig
	make -C linux/src prepare
	make -C linux/src scripts

linux/linux.tar.gz: make_linux
	$(info === Package Linux kernel)
	rm -f $@
	cd $(dir $@) && tar czf $(notdir $@) src

.PHONY: linux-mrproper
linux-mrproper:
	make -C linux/src -j$(shell nproc) mrproper

.PHONY: mrproper
mrproper:: linux-mrproper
