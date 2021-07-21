#!/usr/bin/env python3
# 
# Copyright 2021 Intel Corporation
# SPDX-License-Identifier: MIT
#
# File: cc_nistjuliet_auto.py
#
# Description: Runs all CWE binaries sub-directries (recursively) in native and
# CC-enabled mode and collects results in a CSV file.
#
# To use, run scons to compile the workloads (see SConstruct) and then invoke 
# this file './cc_nistjuliet_auto.py'
#
#

import os
import sys
import multiprocessing as mp
import subprocess
import csv
import re
import itertools
import argparse

def enumerateCweBinariesInDir(wd):
    files = next(os.walk(wd))[2] # Get list of files in working directory
    files = [fl for fl in files if fl.endswith(".out")] # Filter only *.out
    return [wd + "/" + file for file in files]
    
def enumerateCweBinaries(wd="."):
    dirs = next(os.walk(wd))[1] + ['.'] # Get list of directories and subdirectories from the working directory
    ret = []
    for dir in dirs:
        ret.extend(enumerateCweBinariesInDir(dir)) # Recurse through all directories
    return ret

def stripWorkloadDecorators(string):
    regex = r"Calling (?:good|bad)\(\)\.\.\.\n(.*)\nFinished (?:good|bad)\(\)"
    matches = re.search(regex, string, re.DOTALL)
    if matches:
        return matches.group(1)
    else:
        return string

def stripCCDebugInfo(string):
    string = string.replace("Initializing CC_MMAP!\n","")
    return string

def cleanup_results(results):
    (stdout, stderr, returncode) = results
    stdout = stripWorkloadDecorators(stdout)
    stderr = stripCCDebugInfo(stderr)
    if "ERROR" in stdout and not returncode:
        returncode = -9998
    return((stdout, stderr, returncode))

def runbinary(binary, env):
    stdout = ""
    stderr = ""
    returncode = 0
    try:
        sp = subprocess.run(binary, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8', errors="replace", timeout=30) # Allow 30 seconds for every program.
        stdout = sp.stdout
        stderr = sp.stderr
        returncode = sp.returncode
    except subprocess.TimeoutExpired as e:
        returncode = -9999
        stdout = e.stdout
        stderr = e.stderr
    return((stdout, stderr, returncode))

def patchelf(binary):
    print("Patching " + binary)
    os.system("cp " + binary + " " + binary + "_glibc_230")
    os.system("patchelf --set-interpreter /home/simics/glibc/glibc-2.30_install/lib/ld-2.30.so --set-rpath /home/simics/glibc/glibc-2.30_install/lib " + binary + "_glibc_230")

def binaryWorkerThread(binary): # Run in parrallel for each binary in run list
    goodoutput = cleanup_results(runbinary(binary + "_glibc_230", os.environ))
    badoutput =  cleanup_results(runbinary(binary + ".bad_glibc_230", os.environ))
    
    print("Ran " + binary)
    return [binary] + list(goodoutput) + list(badoutput)

def exportResults(results, file):
    with open(file,'w') as resultfile:
        csvWriter = csv.writer(resultfile,delimiter=',')
        csvWriter.writerow(["Benchmark Name", "good stdout", "good stderr", "good return code", "bad stdout", "bad stderr", "bad return code"]) # This needs to match the 'return' order from 6 rows above
        csvWriter.writerows(results)

def runCweBinaries(binariesToRun):
    pool = mp.Pool(16) # set to the number of workloads to run in parrallel
    list_of_results = pool.map(binaryWorkerThread, binariesToRun)
    pool.close()
    pool.join()
    return list_of_results

def main():
  step = sys.argv[1] if len(sys.argv) > 1 else None
  print(step)
  
  binariesToRun = enumerateCweBinaries()
  print("Found " + str(len(binariesToRun)) + " binaries")
  #Uncomment to run a single binary:
  #binariesToRun = [binariesToRun[0]]
  
  if step is None or step == "patchelf":
    [patchelf(binary) for binary in binariesToRun]
    [patchelf(binary+ ".bad") for binary in binariesToRun]
    print("Patched " + str(len(binariesToRun)) + "x2 binaries")
    
  if step is None or step == "runCweBinaries":
    #Uncomment to run the same binary a bunch of times:
    #binariesToRun = itertools.repeat(binariesToRun[0], 100)
    list_of_results = runCweBinaries(binariesToRun)
    exportResults(list_of_results, 'cc_auto_results.csv')  
    print("Found " + str(len(binariesToRun)) + " binaries")
    print("Saved " + str(len(list_of_results)) + " results to 'cc_auto_results.csv'")
  
if __name__== "__main__":
  main()
