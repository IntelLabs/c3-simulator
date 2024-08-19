#! /bin/bash
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

# Run with VERBOSE=1 scripts/demo/clang_tidy.sh
# For output to stdout
LOG_FILE=/dev/null
if [ -n "${VERBOSE}" ]; then
  LOG_FILE=/dev/tty
fi

# Dir containing original tests
TEST_DIR="unit_tests/integrity"
# Tmpdir that it writes all test files to
TEST_WORK_DIR="tmp_clang_tidy_test"

function do_test {

# Args
cpp_file=$1
expected_output=$2

# Run test case in simics and grep for output
RESULT=$(echo "exit" | ./simics --no-win -no-gui unit_tests/runtest_common.simics \
    src_path=$TEST_WORK_DIR \
    src_file=$cpp_file \
    workload_name=a.out \
    model=c3 \
    enable_integrity=TRUE \
    checkpoint=checkpoints/cc_kernel.ckpt \
    compiler="/home/simics/llvm/llvm_install/bin/clang++" \
    include_folders=unit_tests/include/unit_tests \
    env_vars="LD_LIBRARY_PATH=/home/simics/glibc/glibc-2.30_install/lib:/home/simics/llvm/llvm_install/lib" \
    gcc_flags="-ldl -lm -lpthread -pthread -fuse-ld=lld -finsert-intraobject-tripwires=all -Iinclude -DC3_MODEL=cc" 2>&1 | tee $LOG_FILE | grep -o "$expected_output" | head -n 1)
if [ "$RESULT" != "$expected_output" ]; then
   echo "Error: Output of $cpp_file did not contain '$expected_output'"
   exit 1
fi
echo "  Output contains '$expected_output'."
echo "  [x] OK"

}

function do_clang_tidy {
cpp_file=$TEST_WORK_DIR/$1
# True if expected len of diff > 0, false if no diff expected
expect_diff=$2
cpp_file_clang_tidy=${cpp_file%.cpp}.clang-tidy.cpp
cp $cpp_file $cpp_file_clang_tidy
llvm/llvm_install/bin/clang-tidy --checks=-*,misc-c3-* --fix-errors --fix-notes $cpp_file_clang_tidy > $LOG_FILE 2>&1

DIFF_OUT="$(diff -U 2 $cpp_file $cpp_file_clang_tidy)"

echo "  Diff introduced by clang-tidy:"
echo "$DIFF_OUT" | sed 's/^/    /'
if [ "${expect_diff}" = true ]; then
	if [ -z "$DIFF_OUT" ]; then
          echo "[ERROR] Expected non-zero-length diff. Something went wrong with clang-tidy."
	  exit 1
	fi
echo "  Got non-zero-length diff"
echo "  [x] OK"
else
	if [ -n "$DIFF_OUT" ]; then
          echo "[ERROR] Expected zero-length diff. Something went wrong with clang-tidy."
	  exit 1
	fi
echo "  Got zero-length diff"
echo "  [x] OK"
fi
}

function do_cleanup {
cpp_file=$TEST_WORK_DIR/$1
cpp_file_clang_tidy=${cpp_file%.cpp}.clang-tidy.cpp
rm $cpp_file_clang_tidy
}

function do_setup_test {
cpp_file=$TEST_DIR/$1
cp $cpp_file $TEST_WORK_DIR
}

# Setup workdir where script stores the files
mkdir $TEST_WORK_DIR

echo "TEST SCRIPT for C3 clang-tidy rule for struct array field rearranging"
echo ""
echo "1. Run base case (unit_tests/integrity/icv_intra_obj_tripwire_multibuf_bad.cpp). Tripwires should detect an issue."
do_setup_test "icv_intra_obj_tripwire_multibuf_bad.cpp"
do_test "icv_intra_obj_tripwire_multibuf_bad.cpp" "Fault on write ICV fail"
echo "2. Now running clang-tidy to produce icv_intra_obj_tripwire_multibuf_bad.clang-tidy.cpp with buffers moved adjacent to each other."
do_clang_tidy "icv_intra_obj_tripwire_multibuf_bad.cpp" true
echo ""
echo "3. Run rewritten case (icv_intra_obj_tripwire_multibuf_bad.clang-tidy.cpp). Tripwires should, again, detect an issue. (Should print 'Fault on write ICV fail' on next line)"
do_test "icv_intra_obj_tripwire_multibuf_bad.clang-tidy.cpp" "Fault on write ICV fail"
echo ""

echo "4. Run base case without bug (icv_intra_obj_tripwire_multibuf_good.cpp)."
do_setup_test "icv_intra_obj_tripwire_multibuf_good.cpp"
do_test "icv_intra_obj_tripwire_multibuf_good.cpp" "Done with juliet_good"
echo ""
echo "5. Now running clang-tidy to produce icv_intra_obj_tripwire_multibuf_good.clang-tidy.cpp with buffers moved adjacent to each other."
do_clang_tidy "icv_intra_obj_tripwire_multibuf_good.cpp" true
echo ""
echo "6. Run re-written good test case (icv_intra_obj_tripwire_multibuf_good.clang-tidy.cpp)."
do_test "icv_intra_obj_tripwire_multibuf_good.clang-tidy.cpp" "Done with juliet_good"
echo ""

echo "7. Run test case with three buffers (icv_intra_obj_tripwire_multibuf_three.cpp)."
do_setup_test "icv_intra_obj_tripwire_multibuf_three.cpp"
do_test "icv_intra_obj_tripwire_multibuf_three.cpp" "Fault on write ICV fail"
echo ""
echo "8. Run clang-tidy on icv_intra_obj_tripwire_multibuf_three.cpp"
do_clang_tidy "icv_intra_obj_tripwire_multibuf_three.cpp" true
echo ""
echo "9. Run re-written icv_intra_obj_tripwire_multibuf_three.clang-tidy.cpp"
do_test "icv_intra_obj_tripwire_multibuf_three.clang-tidy.cpp" "Fault on write ICV fail"
echo ""

echo "10. Run test case with buffer at end of struct (icv_intra_obj_tripwire_multibuf_end.cpp)."
do_setup_test "icv_intra_obj_tripwire_multibuf_end.cpp"
do_test "icv_intra_obj_tripwire_multibuf_end.cpp" "Fault on write ICV fail"
echo ""
echo "11. Run clang-tidy on icv_intra_obj_tripwire_multibuf_end.cpp"
do_clang_tidy "icv_intra_obj_tripwire_multibuf_end.cpp" true
echo ""
echo "12. Run re-written icv_intra_obj_tripwire_multibuf_end.clang-tidy.cpp"
do_test "icv_intra_obj_tripwire_multibuf_end.clang-tidy.cpp" "Fault on write ICV fail"
echo ""

echo "13. Run test case with adjacent buffers (icv_intra_obj_tripwire_multibuf_adjacent.cpp)."
do_setup_test "icv_intra_obj_tripwire_multibuf_adjacent.cpp"
do_test "icv_intra_obj_tripwire_multibuf_adjacent.cpp" "Fault on write ICV fail"
echo ""
echo "14. Run clang-tidy on icv_intra_obj_tripwire_multibuf_adjacent.cpp (expect no suggested changes)"
do_clang_tidy "icv_intra_obj_tripwire_multibuf_adjacent.cpp" false
echo ""
echo "15. Run re-written icv_intra_obj_tripwire_multibuf_adjacent.clang-tidy.cpp"
do_test "icv_intra_obj_tripwire_multibuf_end.clang-tidy.cpp" "Fault on write ICV fail"
echo ""



echo "Performing cleanup and exiting."
do_cleanup "icv_intra_obj_tripwire_multibuf_bad.cpp"
do_cleanup "icv_intra_obj_tripwire_multibuf_good.cpp"
do_cleanup "icv_intra_obj_tripwire_multibuf_three.cpp"
do_cleanup "icv_intra_obj_tripwire_multibuf_end.cpp"
do_cleanup "icv_intra_obj_tripwire_multibuf_adjacent.cpp"
rm -r $TEST_WORK_DIR
echo "[DONE]"
