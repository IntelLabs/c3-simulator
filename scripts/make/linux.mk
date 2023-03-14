
install_dependencies_ubuntu::
	$(info === Depencencies for Linux builds)
	sudo apt install -y \
		build-essential \
		bison \
		dwarves \
		flex \
		libelf-dev \
		libssl-dev \
		llvm

.PHONY: make_linux
make_linux: linux/src/.config
	$(info === Build Linux kernel)
	make -C linux/src -j$(shell nproc)

linux/src/.config: linux/default_config
	cp -f $< $@

linux/linux.tar.gz: make_linux
	$(info === Package Linux kernel)
	rm -f $@
	cd $(dir $@) && tar czf $(notdir $@) src

.PHONY: clean-linux
mrproper::
	make -C linux/src -j$(shell nproc) mrproper
