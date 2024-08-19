#!/usr/bin/bash

# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

# Enter C3 repository root dir
current_script_path=$(readlink -f "${BASH_SOURCE[0]}")
c3_path=$(sed 's/\/scripts\/demo\/.*//' <<< ${current_script_path})
pushd $c3_path

# Download NIST Juliet test-suite
juliet_test_suite=2017-10-01-juliet-test-suite-for-c-cplusplus-v1-3.zip
juliet_download_link=https://samate.nist.gov/SARD/downloads/test-suites/${juliet_test_suite}
nist_juliet_path=tests/nist-juliet
juliet_suite_path=${nist_juliet_path}/test-suite
juliet_script_path=${nist_juliet_path}/scripts
juliet_result_path=${nist_juliet_path}/results
if [ ! -d ${juliet_suite_path} ] ; then
  if [ ! -f "${nist_juliet_path}/${juliet_test_suite}" ] ; then
    echo "Downloading NIST Juliet test suite ... "
    wget ${juliet_download_link} -P ${nist_juliet_path}/
  fi
  echo "Unpacking NIST Juliet test suite ... "
  unzip -d ${nist_juliet_path}/ -q ${nist_juliet_path}/${juliet_test_suite}
  mv ${nist_juliet_path}/C ${juliet_suite_path}
  patch -N -p 0 ${juliet_suite_path}/testcasesupport/io.c < $c3_path/scripts/demo/cwe457_random.patch
fi

# Check for the presence of simics scripts
if [ ! -f ./simics ]
then
  echo
  echo "./simics not found. Please run from the C3 repository root dir"
  exit 1
fi

testpath=$juliet_suite_path/testcases/CWE457_Use_of_Uninitialized_Variable
testsupport=$juliet_suite_path/testcasesupport

# Build a list of relevant testcases (CWE457 that use malloc)
listfile=$(mktemp testlist.XXXXXX)
grep -RniE \
  "malloc *\(" $testpath \
    | sed -e "s@$testpath/@@" \
    | sed -e 's@:.*@@' \
    | sort -u \
    > $listfile
echo
echo "Running the test cases"
echo
for testfile in $(cat $listfile)
do
  # Test case selection
  fullpath=$testpath/$testfile
  src_path=$(dirname $fullpath)
  src_file=$(basename $fullpath)
  if [ ! -f $src_path/$src_file ]
  then
    echo "Error parsing testcase list:"
    echo "  src_path: $src_path"
    echo "  src_file: $src_file"
  fi

  # Skip test files that only call malloc in the GOOD configuration
  if [[ $src_file == *pointer* ]]
  then
    continue
  fi

  # Merge test cases that need multiple files
  unset mergefile
  if [[ $src_file == *a.c ]] || [[ $src_file == *a.cpp ]]
  then
    src_root=$(echo $src_file | sed -e 's/a\.cp\?p\?$//')
    if [[ $src_file == *a.cpp ]]
    then
      mergefile=$(mktemp merge.XXXXXX.cpp)
    else
      mergefile=$(mktemp merge.XXXXXX.c)
    fi
    echo "Merging $src_file and dependencies into $mergefile"
    cat ${src_path}/${src_root}* > $mergefile
    src_path=$(dirname $mergefile)
    src_file=$(basename $mergefile)
  fi

  # Test case configuration
  testcfg=""
  testcfg="$testcfg src_path=$src_path"
  testcfg="$testcfg src_file=$src_file"
  testcfg="$testcfg include_folders=$testsupport/"
  testcfg="$testcfg workload_name=app"

  # Simics configuration
  simcfg=""
  simcfg="$simcfg model=c3"
  simcfg="$simcfg enable_integrity=TRUE"
  simcfg="$simcfg integrity_break_on_read_mismatch=FALSE"
  simcfg="$simcfg integrity_fault_on_read_mismatch=FALSE"
  simcfg="$simcfg integrity_break_on_write_mismatch=FALSE"
  simcfg="$simcfg integrity_fault_on_write_mismatch=FALSE"
  simcfg="$simcfg checkpoint=checkpoints/cc_kernel.ckpt"

  # Compiler configuration
  cccfg=""
  cccfg="$cccfg compiler=clang"

  # Run simics
  ./simics \
    scripts/runworkload_common.simics \
    $testcfg \
    $simcfg \
    $cccfg \
    gcc_flags="-mpreiniticv -DOMITGOOD -DINCLUDEMAIN -Iinclude include/io.c" \
    exit=TRUE

  # Delete the merged file if it has been created
  if [ -v mergefile ]
  then
    rm -f $mergefile
  fi
done

rm -f $listfile

popd
