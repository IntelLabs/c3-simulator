# Copyright 2023-2024 Intel Corporation
# SPDX-License-Identifier: MIT

.PHONY: documentation
documentation: doxygen-docs

.PHONY: doxygen-docs
doxygen-docs:
	doxygen doxygen.config

install_dependencies_ubuntu::
	$(info === Doxygen dependencies)
	sudo apt install -y \
		doxygen \
		graphviz
