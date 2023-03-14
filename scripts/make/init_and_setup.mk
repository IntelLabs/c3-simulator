
include scripts/make/xed.mk

SIMICS_BIN ?= /opt/simics/simics-6/simics-latest/bin

PATCHELF_PKG = patchelf-0.10.tar.gz
PATCHELF_URL = https://codeload.github.com/NixOS/patchelf/tar.gz/refs/tags/0.10

FILES_TO_DECOMPRESS_Z += $(wildcard internal/spec/x264_17/run/BuckBunny.yuv.zst)
FILES_TO_DECOMPRESS_Z += $(wildcard internal/spec/parest17/run/parest17.zst)
FILES_TO_DECOMPRESS_Z += $(wildcard internal/spec/soplex/run/ref.mps.zst)
FILES_TO_DECOMPRESS = $(FILES_TO_DECOMPRESS_Z:%.zst=%)

.PHONY: simics_setup
simics_setup: patchelf-0.10.tar.gz $(FILES_TO_DECOMPRESS) simics install-xed

simics:
	$(SIMICS_BIN)/project-setup --ignore-existing-files .

$(PATCHELF_PKG):
	curl -o $@ $(PATCHELF_URL)

$(FILES_TO_DECOMPRESS) : % : %.zst
	test -e $@ || zstd -d $^

install_dependencies_ubuntu::
	$(info === Depencencies for decompressing large files)
	sudo apt install -y \
		zstd

mrproper::
	rm -f $(FILES_TO_DECOMPRESS) $(PATCHELF_PKG)
