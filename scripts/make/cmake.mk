# Copyright 2023 Intel Corporation
# SPDX-License-Identifier: MIT

ifndef SCRIPTS_MAKE_CMAKE_MK_
SCRIPTS_MAKE_CMAKE_MK_ =1

cmake-version = 3.26.4
cmake-url = https://github.com/Kitware/CMake/releases/download/v$(cmake-version)/cmake-$(cmake-version).tar.gz
cmake-tar = cmake-$(cmake-version).tar.gz

lib/cmake-$(cmake-version).tar.gz:
	mkdir -p lib
	cd lib && wget $(cmake-url) 

lib/cmake-$(cmake-version): lib/cmake-$(cmake-version).tar.gz
	tar xf lib/cmake-$(cmake-version).tar.gz -C lib

install-cmake: lib/cmake-$(cmake-version)
	cd lib/cmake-$(cmake-version) && ./bootstrap --parallel=$J -- -DCMAKE_USE_OPENSSL=OFF && make -j$J

mrproper::
	rm -rf lib/cmake-$(cmake-version).tar.gz lib/cmake-$(cmake-version)

endif
