# Copyright 2023-2024 Intel Corporation
# SPDX-License-Identifier: MIT

.PHONY: compile_commands.json
compile_commands.json:
	make clean; bear make $(nproc)
	cat compile_commands.json | \
		sed 's#"-I.",#"-I.", "-I'$(pwd)/modules/common'",#' | \
		sed 's#"-I.",#"-I.", "-I/opt/simics/xed/xed/obj/wkit/include/xed",#' | \
		jq > compile_commands.json.new
	mv -f compile_commands.json.new compile_commands.json
