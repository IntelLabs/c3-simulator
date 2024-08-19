#!/usr/bin/bash

# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

runlog=$(mktemp debug/cwe457.XXXXXX.log)
analog=$(mktemp debug/cwe457.XXXXXX.log)
runpath=$(dirname $0)

runscript=$runpath/cwe457_run.sh
if [ ! -f $runpath/cwe457_run.sh ]
then
  echo "Script not found ($runscript)"
  exit 1
fi
echo
echo "Running NIST Juliet tests for Uninitialized Read (CWE457)"
echo "  (log file: $runlog)"
$runscript &> $runlog

analyzescript=$runpath/cwe457_analyze.py
if [ ! -f $analyzescript ]
then
  echo "Scriptf not found ($analyzescript)"
  exit 1
fi
echo
echo "Analysing the logs for uninitialized read detection"
echo "  (log file: $analog)"
$analyzescript -i $runlog &> $analog

testtotal=$(grep "Running model" $runlog | wc -l)
testfail=$(grep "no uninitialized read detected" $analog | wc -l)
testskip=$(grep "failed to build" $analog | wc -l)

testfailf="$testfail.0"
testskipf="$testskip.0"

echo
echo "Total number of tests ran: $testtotal"
python3 -c "print('Skipped %d tests (%3.2f%%)' % ($testskip, (100*$testskipf/$testtotal)))"
python3 -c "print('Failed %d tests (%3.2f%%)' % ($testfail, (100*$testfailf/$testtotal)))"
if [ $testfail -ne 0 ]
then
  cat $analog
fi
