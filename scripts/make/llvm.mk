
.PHONY: make_llvm
make_llvm:
	$(info === Build LLLVM)
	./llvm/make_llvm.sh

make_llvm-lldb:
	$(info === Build LLLVM (with LLDB))
	CC_LLVM_LLDB=1 ./llvm/make_llvm.sh

mrproper::
	rm -rf llvm/llvm_build llvm/llvm_install

install_dependencies_ubuntu::
	$(info === glibc dependencies)
	sudo apt install -y \
		cmake \
		ninja-build
