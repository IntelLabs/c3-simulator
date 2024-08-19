#!/bin/bash
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

#set -e

#usage: ./run_juliet.sh -m <model> | h <heap protection> | s <stack protection> | a <align allocation> | w <write output> | e <environment variables>
# -m <model>; model can be "c3" | "native"
# -h <heap protection>; 0 or 1 to run testcases for heap protection
# -s <stack protection>; 0 or 1 to run testcases for stack protection
# -a <align allocation>; 0 or 1 to align allocation to the right
# -w <write output>; 0 or 1 to write the testcase summary to an excel file
# -e <env_vars>; specify any environment variable. not used now

# Run Juliet heap testcases without heap protection
#	./tests/nist-juliet/scripts/run_juliet.sh -m "native"
# Run Juliet stack testcases without stack protection
# 	./tests/nist-juliet/scripts/run_juliet.sh -m "native" -s 1
# Run Juliet heap testcases with C3 heap protection without right alignment .
#	./tests/nist-juliet/scripts/run_juliet.sh -m "c3" -h 1
# Run Juliet heap testcases with C3 heap protection + right alignment
#	./tests/nist-juliet/scripts/run_juliet.sh -m "c3" -h 1 -a 1
# Run Juliet stack testcases with C3 stack protection
#	./tests/nist-juliet/scripts/run_juliet.sh -m "c3" -s 1

#sudo apt update
#sudo apt-get -y install unzip wget python3 python3-pip
#sudo pip3 install pandas openpyxl

current_script_path=$(readlink -f "${BASH_SOURCE[0]}")
simics_path=$(sed 's/\/tests\/.*//' <<< ${current_script_path})

juliet_test_suite=2017-10-01-juliet-test-suite-for-c-cplusplus-v1-3.zip
juliet_download_link=https://samate.nist.gov/SARD/downloads/test-suites/${juliet_test_suite}

nist_juliet_path=tests/nist-juliet
juliet_suite_path=${nist_juliet_path}/test-suite
juliet_script_path=${nist_juliet_path}/scripts
juliet_result_path=${nist_juliet_path}/results

simics_script=${juliet_script_path}/nist-juliet.simics

########## Simics parameters to set ##########
declare -a cwe_names

env_vars="\"\""
build=1
write_output=1
model=""
align_right=0
protect_stack=0
protect_heap=0

while getopts m:h:s:a:w:e flag
do
    case "${flag}" in
        m) model=${OPTARG};;
        h) protect_heap=${OPTARG};;
        s) protect_stack=${OPTARG};;
        a) align_right=${OPTARG};;
        w) write_output=${OPTARG};;
		e) env_vars=${OPTARG};;
    esac
done
if [ $OPTIND -eq 1 ]; then
	echo "No options were passed";
	model="c3"
	align_right=1
	protect_heap=1
fi

ckpt_name=cc_kernel
if [ $model  = "c3" ]; then
	if [ ${align_right}  = "1" ]; then
		ckpt_name=cc_kernel_1b_ovf
	fi
fi

ckpt=checkpoints/${ckpt_name}.ckpt
args="checkpoint=$ckpt upload_juliet=1"

if [ $model  = "native" ]; then
	if [ ${protect_stack}  = "1" ]; then
		model="c3"
		args="$args model=$model disable_cc_env=1 disable_cc_ptr_encoding=1"
		cwe_names=("CWE122_Stack")
	elif [ ${protect_stack}  = "0" ]; then
		args="$args model=$model"
		cwe_names=("CWE122_Heap_Based_Buffer_Overflow" "CWE416_Use_After_Free" "CWE457_Use_of_Uninitialized_Variable")
	fi
elif [ $model  = "c3" ]; then
	args="$args model=$model"
	if [ ${protect_heap}  = "1" ]; then
		args="$args enable_integrity=1"
		cwe_names=("CWE122_Heap_Based_Buffer_Overflow" "CWE416_Use_After_Free" "CWE457_Use_of_Uninitialized_Variable")
	elif [ ${protect_stack}  = "1" ]; then
		args="$args disable_cc_env=1 enable_castack=1 unwinder=llvm_libunwind"
		cwe_names=("CWE122_Stack")
	fi
fi

if [ $build  = "1" ]; then
	args="$args build=1"
fi

if [ ${write_output}  = "1" ]; then
	args="$args write_output=1"
fi
######################################

echo "Simics path: " ${simics_path}
echo "NIST Juliet directory: " ${nist_juliet_path}
echo "Simics args: " ${args}

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
	local cwe_path=${juliet_suite_path}/testcases/${cwe_name}
    echo "CWE: " ${cwe_path}

	# get all the directories
	local dirs=$(get_list_of_dirs ${cwe_path})
	for d in $dirs ; do
	   local makefile=${d}/Makefile
	   echo "Makefile: " ${makefile}
	   modify_makefile ${makefile}
    done

	return 0
}

# Write outputs of testcase(s) execution to an excel file
write_csv_to_excelsheet (){
	local csvfile="$1"
	local excelfile="$2"
	local excelsheet="$3"

    cd ${juliet_script_path}
	local python_command="import process_output;  process_output.write_csv_to_excelsheet('$csvfile', '$excelfile', '$excelsheet')"
	python3 -c "${python_command}"
	cd ${simics_path}
}

add_csv_to_excelsheet (){
	local csvfile="$1"
	local excelfile="$2"
	local excelsheet="$3"

    cd ${juliet_script_path}
	local python_command="import process_output;  process_output.add_csv_to_excelsheet('$csvfile', '$excelfile', '$excelsheet')"
	python3 -c "${python_command}"
	cd ${simics_path}
}

# Compute C3 detection rate
summarize_cwe_detection(){
	local excelfile="$1"

	cd ${juliet_script_path}
	local python_command="import process_output;  process_output.summarize_results('$excelfile')"
	python3 -c "${python_command}"
	cd ${simics_path}
}

# Run the testcases
run_cwe_testcases() {
	local cwe_full_name="$1"
	local cwe_path=${juliet_suite_path}/testcases/${cwe_full_name}
    local cwe_no="$(echo ${cwe_full_name} | awk -F'_' '{print $1}')"

	# get all the directories
	count=0
    local dirs=$(get_list_of_dirs ${cwe_path})
    local output_file=${simics_path}/${juliet_result_path}/${cwe_no}/${cwe_no}
	if [ ${protect_stack} = "1" ]; then
		output_file="${output_file}_stack"
	fi
	output_file="${output_file}.xlsx"

	local temp_file=${simics_path}/${juliet_result_path}/${cwe_no}/temp.csv

	rm -f ${output_file}
	rm -f ${temp_file}
	mkdir -p ${juliet_result_path}/${cwe_no}
	echo "Testcases output to: " ${output_file}

	for d in $dirs ; do
        local dname="$(basename $d)"
		echo "Running testcases from: " $d

	    cd ${simics_path}
		./simics ${simics_script} workload_dir=$d ${args} env_vars=${env_vars} result_file=${temp_file}

		if [ ${write_output} = "1" ]; then
			if (( $count==0 )); then
				write_csv_to_excelsheet ${temp_file} ${output_file} ${dname}
			else
				add_csv_to_excelsheet ${temp_file} ${output_file} ${dname}
			fi
			rm ${temp_file}
			((count+=1))
		fi
    done

	summarize_cwe_detection ${output_file}
	return 0
}

#download NIST Juliet test-suite
if [ ! -d ${juliet_suite_path} ] ; then
    if [ ! -f "${nist_juliet_path}/${juliet_test_suite}" ] ; then
	    echo "Downloading NIST Juliet test suite ... "
	    curl -o ${nist_juliet_path}/${juliet_test_suite} ${juliet_download_link}
	fi

    echo "Unpacking NIST Juliet test suite ... "
	unzip -d ${nist_juliet_path}/ -q ${nist_juliet_path}/${juliet_test_suite}
	mv ${nist_juliet_path}/C ${juliet_suite_path}
	patch -N -p 0 ${juliet_suite_path}/testcasesupport/io.c < ${juliet_script_path}/io_random.patch
fi

# Generate the simics checkpoint
cd ${simics_path}
if [ ! -d "$ckpt" ]; then
   echo "Generating checkpoint ${ckpt_name}.ckpt ... "
   make ckpt-${ckpt_name}
fi

if [ ${protect_stack} = 1 ]; then
	src_cwe_path=${juliet_suite_path}/testcases/CWE122_Heap_Based_Buffer_Overflow/s06
	dest_cwe_path=${juliet_suite_path}/testcases/CWE122_Stack
	echo "src_path = ${src_cwe_path}"
	echo "dest_path = ${dest_cwe_path}"
	rm -rf ${dest_cwe_path}

	#copy testcases
	mkdir -p ${dest_cwe_path}
	cp ${src_cwe_path}/CWE122_Heap_Based_Buffer_Overflow__c_CWE129_fgets_[0,2,3][0-9].c* ${dest_cwe_path}
	cp ${src_cwe_path}/CWE122_Heap_Based_Buffer_Overflow__c_CWE129_fgets_1[0-1,3-9].c* ${dest_cwe_path}
	cp ${src_cwe_path}/CWE122_Heap_Based_Buffer_Overflow__c_CWE129_fgets_4[1-3].c* ${dest_cwe_path}

	cp ${src_cwe_path}/main_linux.cpp ${dest_cwe_path}
	cp ${src_cwe_path}/main.cpp ${dest_cwe_path}
	cp ${src_cwe_path}/Makefile ${dest_cwe_path}
	cp ${src_cwe_path}/testcases.h ${dest_cwe_path}

	#patch source files
	patch -p1 -d ${dest_cwe_path}/ < ${juliet_script_path}/stack.patch

	#set cwe name
	cwe_names=("CWE122_Stack")
fi

#patch CWE Makefiles
echo "Patching CWE makefiles ... "
for cwe in ${cwe_names[@]} ; do
	generate_c3_makefile ${cwe}
done

#run the CWE testcases
echo "Running CWE testcases ... "
for cwe in ${cwe_names[@]} ; do
	run_cwe_testcases ${cwe}
done
