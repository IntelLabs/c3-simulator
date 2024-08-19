#!/bin/bash
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

current_script_path=$(readlink -f "${BASH_SOURCE[0]}")
simics_path=$(sed 's/\/tests\/.*//' <<< ${current_script_path})
nist_juliet_path=tests/nist-juliet
juliet_suite_path=${nist_juliet_path}/test-suite

echo "Simics path: " ${simics_path}

# Get all the (sub)directories of testcases
get_list_of_dirs(){
	local dir_path="$1"
	local dirs=`ls -d ${dir_path}/*/`>/dev/null 2>&1
	if [ "$dirs" ]; then
	    echo $dirs
	else
		echo ${dir_path}
    fi
}

# Patch the Makefiles to enable C3 protection
modify_makefile () {
    local makefile="$1"
	local model="$2"
	local protect_stack="$3"

	c3_makefile=${makefile}-c3
	cp ${makefile} ${c3_makefile}

	if [ ${protect_stack}  = "1" ]; then
		sed -i 's/^CC=.*/CC=\/usr\/bin\/g++/' ${c3_makefile}

		sed -i 's/^CFLAGS=.*/CFLAGS=-c -O0 -g -gdwarf/' ${c3_makefile}
		sed -i '/^CFLAGS=-c.*/a CFLAGS-c3=-O0 -g -gdwarf' ${c3_makefile}
		sed -i 's/^LFLAGS=.*/LFLAGS=-ldl -lpthread -lm -pthread/' ${c3_makefile}

		sed -i 's/INCLUDES=-I.*/INCLUDES=-I ..\/..\/testcasesupport/' ${c3_makefile}
		sed -i 's/C_SUPPORT_PATH=.*/C_SUPPORT_PATH=..\/..\/testcasesupport\//' ${c3_makefile}

		sed -i '/^$(INDIVIDUALS_C): $(C_SUPPORT_OBJECTS)/{n;s/.*/\t$(CC) $(CFLAGS-c3) $(INCLUDES) $(INCLUDE_MAIN) -DOMITBAD -o $@ $(wildcard $(subst .out,,$@)*.c) $(C_SUPPORT_OBJECTS) $(LFLAGS)\n\t$(CC) $(CFLAGS-c3) $(INCLUDES) $(INCLUDE_MAIN) -DOMITGOOD -o $@.bad $(wildcard $(subst .out,,$@)*.c) $(C_SUPPORT_OBJECTS) $(LFLAGS)/'} ${c3_makefile}
		sed -i '/^$(INDIVIDUALS_CPP): $(C_SUPPORT_OBJECTS)/{n;s/.*/\t$(CPP) $(CFLAGS-c3) $(INCLUDES) $(INCLUDE_MAIN) -DOMITBAD -o $@ $(wildcard $(subst .out,,$@)*.cpp) $(C_SUPPORT_OBJECTS) $(LFLAGS)\n\t$(CPP) $(CFLAGS-c3) $(INCLUDES) $(INCLUDE_MAIN) -DOMITGOOD -o $@.bad $(wildcard $(subst .out,,$@)*.cpp) $(C_SUPPORT_OBJECTS) $(LFLAGS)/'} ${c3_makefile}

	elif [ $model  = "native" ]; then
		sed -i '/^$(INDIVIDUALS_C): $(C_SUPPORT_OBJECTS)/{n;s/.*/\t$(CC) $(INCLUDES) $(INCLUDE_MAIN) -DOMITBAD -o $@ $(wildcard $(subst .out,,$@)*.c) $(C_SUPPORT_OBJECTS) $(LFLAGS)\n\t$(CC) $(INCLUDES) $(INCLUDE_MAIN) -DOMITGOOD -o $@.bad $(wildcard $(subst .out,,$@)*.c) $(C_SUPPORT_OBJECTS) $(LFLAGS)/'} ${c3_makefile}
		sed -i '/^$(INDIVIDUALS_CPP): $(C_SUPPORT_OBJECTS)/{n;s/.*/\t$(CPP) $(INCLUDES) $(INCLUDE_MAIN) -DOMITBAD -o $@ $(wildcard $(subst .out,,$@)*.cpp) $(C_SUPPORT_OBJECTS) $(LFLAGS)\n\t$(CPP) $(INCLUDES) $(INCLUDE_MAIN) -DOMITGOOD -o $@.bad $(wildcard $(subst .out,,$@)*.cpp) $(C_SUPPORT_OBJECTS) $(LFLAGS)/'} ${c3_makefile}

	elif [ $model  = "c3" ]; then
		sed -i 's/^CC=.*/CC=\/home\/simics\/llvm\/llvm_install\/bin\/clang/' ${c3_makefile}
		sed -i 's/^CPP=.*/CPP=\/home\/simics\/llvm\/llvm_install\/bin\/clang++/' ${c3_makefile}
		sed -i 's/^CFLAGS=.*/CFLAGS=-c -mpreiniticv -finsert-intraobject-tripwires=all/' ${c3_makefile}
		sed -i '/^CFLAGS=-c.*/a CFLAGS-c3=-mpreiniticv -finsert-intraobject-tripwires=all' ${c3_makefile}

		sed -i 's/^LFLAGS=.*/LFLAGS=-fuse-ld=lld -ldl -lpthread -lm -pthread/' ${c3_makefile}

		sed -i '/^$(INDIVIDUALS_C): $(C_SUPPORT_OBJECTS)/{n;s/.*/\t$(CC) $(CFLAGS-c3) $(INCLUDES) $(INCLUDE_MAIN) -DOMITBAD -o $@ $(wildcard $(subst .out,,$@)*.c) $(C_SUPPORT_OBJECTS) $(LFLAGS)\n\t$(CC) $(CFLAGS-c3) $(INCLUDES) $(INCLUDE_MAIN) -DOMITGOOD -o $@.bad $(wildcard $(subst .out,,$@)*.c) $(C_SUPPORT_OBJECTS) $(LFLAGS)/'} ${c3_makefile}
		sed -i '/^$(INDIVIDUALS_CPP): $(C_SUPPORT_OBJECTS)/{n;s/.*/\t$(CPP) $(CFLAGS-c3) $(INCLUDES) $(INCLUDE_MAIN) -DOMITBAD -o $@ $(wildcard $(subst .out,,$@)*.cpp) $(C_SUPPORT_OBJECTS) $(LFLAGS)\n\t$(CPP) $(CFLAGS-c3) $(INCLUDES) $(INCLUDE_MAIN) -DOMITGOOD -o $@.bad $(wildcard $(subst .out,,$@)*.cpp) $(C_SUPPORT_OBJECTS) $(LFLAGS)/'} ${c3_makefile}
	fi

	sed -i '/^clean:/{n;s/.*/\trm -rf *.o *.out* $(TARGET)/'} ${c3_makefile}

	return 0
}

# Modify makefile for C3 testing
generate_c3_makefile () {
	local cwe_name="$1"
	local model="$2"
	local protect_stack="$3"

	local cwe_path=${juliet_suite_path}/testcases/${cwe_name}
    echo "CWE: " ${cwe_path}

	# get all the directories
	local dirs=$(get_list_of_dirs ${cwe_path})
	for d in $dirs ; do
	   local makefile=${d}/Makefile
	   echo "Makefile: " ${makefile}
	   modify_makefile ${makefile} ${model} ${protect_stack}
    done

	return 0
}
