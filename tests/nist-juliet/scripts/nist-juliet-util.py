#!/usr/bin/env python3
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

#
# Copyright (C) 2019 Intel Corporation. All rights reserved.
#
# File: cc_nistjuliet_auto.py
#
# Description: Runs all CWE binaries sub-directories (recursively) in native and
# CC-enabled mode and collects results in a CSV file.
#
# To use, run scons to compile the workloads (see SConstruct) and then invoke
# this file './cc_nistjuliet_auto.py'
#
# Author: Andrew Weiler, Salmin Sultana
#
import os
import sys
import multiprocessing as mp
import subprocess
from subprocess import PIPE,Popen
import csv
import re
import itertools
import argparse
from filter_testcases import filterOutTestcases

total_testcases=0
c3_detected_testcases=0

def enumerateBinariesInDir(path):
    ret = []
    isDir = -1
    if os.path.isfile(path):
        ret.append(path)
        isDir = 0
    elif os.path.isdir(path):
        isDir = 1
        for dirpath, dirs, files in os.walk(path):
            for fl in files:
                if fl.endswith(".out"): # Filter only *.out
                    ret.append(os.path.join(dirpath, fl))
    return ret, isDir

def getInputParameters(binaryName, type):
    # fgets: string
    # fscanf: int
    inputTypes = ['fgets', 'fscanf']
    args = ''

    for input in inputTypes:
        if input in binaryName.lower():
            if input == 'fgets':
                str_input_1 = '11'
                str_input_2 = '12'
                if type=='good':
                    args = "{ echo \"" + str_input_1 + "\"; echo \"" + str_input_2 + "\"; } | " # fixed string input
                elif type=='bad':
                    args = "{ echo \"" + str_input_1 + "\"; } | "

            elif input == 'fscanf':
                int_input_1 = 11
                int_input_2 = 12
                if type=='good':
                    args = "{ echo \"%d" %(int_input_1) + "\"; echo \"%d"  %(int_input_2) + "\"; } | " # fixed string input
                elif type=='bad':
                    args = "{ echo \"%d"  %(int_input_1) + "\"; } | "

    return args

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
    if results==[]:
        return []

    (stdout, stderr, returncode) = results
    stdout = stripWorkloadDecorators(stdout)
    stderr = stripCCDebugInfo(stderr)
    if "ERROR" in stdout and not returncode:
        returncode = -9998
    return((stdout, stderr, returncode))

def runbinary(binary, os_env, simics_env, type, script_path):
    global total_testcases
    global c3_detected_testcases

    stdout = ""
    stderr = ""
    returncode = -1

    ignored_testcases = ['socket', 'rand']

    if any(ele in binary.lower() for ele in ignored_testcases) == True:
        return []

    args = getInputParameters(binary, type)

    try:
        if type=='bad':
            total_testcases+=1

        command = args + " " + simics_env + " " + binary
        print('Running :: \'' + command + '\'')
        sp = Popen(command,env=os_env,stdout=PIPE, stderr=PIPE, encoding='utf-8', errors="replace", shell=True) # Allow 30 seconds for every program.
        stdout, stderr = sp.communicate()
        sp.wait()
        returncode = sp.returncode
    except subprocess.TimeoutExpired as e:
        returncode = -9999
        stdout = e.stdout
        stderr = e.stderr
    except FileNotFoundError as e:
        print("FileNotFoundError > " + e.strerror)
        returncode = e.errno
        stdout = e.stdout
        stderr = e.stderr
    except OSError as e:
        print("OSError > " + e.strerror)
        returncode = e.errno

    if type=='bad': # covers the case e.g. socket, rand testcases that we don't run
        if returncode not in [0, -1]:
            c3_detected_testcases+=1
        elif returncode==0 and "ERROR" in stdout:
            c3_detected_testcases+=1

    # icv map reset
    if "CC_ICV_ENABLED=1" in simics_env:
        ir = Popen(script_path+"/icv_reset",env=os_env, encoding='utf-8', errors="replace", shell=True)
        ir.wait()

    return((stdout, stderr, returncode))

def patchelf(binary, glibc_path, interpreter, rpath, unwinder):
    print("Patching " + binary)

    #glibc_interpreter=glibc_path+"/ld-2.30.so"
    #patchelf --set-interpreter %s/%s --set-rpath %s %s" % [$guest_glibc_lib, $glibc_ld, $rpath, $workload_name])
	#patchelf --add-needed libunwind.so.1 %s" %	[$workload_name])

    os.system("cp " + binary + " " + binary + "_glibc_230")
    if glibc_path != "/usr/lib":
        os.system("patchelf --set-interpreter " + interpreter + " --force-rpath --set-rpath " + rpath + " " + binary + "_glibc_230")
        if unwinder != "default_unwinder":
            os.system("patchelf --add-needed libunwind.so.1 " + binary + "_glibc_230")

def binaryWorkerThread(binary, script_path, simics_env):
    goodoutput = cleanup_results(runbinary(binary + "_glibc_230", os.environ, simics_env, 'good', script_path))
    if (goodoutput ==[]):
        return []
    badoutput =  cleanup_results(runbinary(binary + ".bad_glibc_230", os.environ, simics_env, 'bad', script_path))
    return [binary] + list(goodoutput) + list(badoutput)

def exportResultInCSV(results, file):
    with open(file,'w') as resultfile:
        csvWriter = csv.writer(resultfile,delimiter=',')
        csvWriter.writerow(["Total testcases", total_testcases])
        csvWriter.writerow(["C3 detected testcases", c3_detected_testcases])
        csvWriter.writerow(["Benchmark Name", "good stdout", "good stderr", "good return code", "bad stdout", "bad stderr", "bad return code"]) # This needs to match the 'return' order from 6 rows above
        csvWriter.writerows(results)

def runBinariesInDir(binariesToRun, script_path, simics_env):
    list_of_results=[]
    for b in binariesToRun:
        ret_list = binaryWorkerThread(b, script_path, simics_env)
        if ret_list != []:
            list_of_results.append(ret_list)
    return list_of_results

def main():
    if len(sys.argv) < 5:
        print("Usage: python nist-juliet-util.py <simics_glibc_path> <simics_script_path> <simics_workload_path> <simics_args>")
        exit()

    simics_glibc_path = sys.argv[1]
    simics_script_path = sys.argv[2]
    unwinder = sys.argv[3]
    interpreter = sys.argv[4]
    rpath = sys.argv[5]
    simics_workload_path = sys.argv[6]
    simics_env = ' '.join(sys.argv[7:])
    print("Workload path: " + simics_workload_path)
    print("Simics env: " + simics_env)

    binariesToRun, isDir = enumerateBinariesInDir(simics_workload_path)
    print("Found " + str(len(binariesToRun)) + " binaries")

    if isDir == 1:
        binariesToRun = filterOutTestcases(simics_workload_path, binariesToRun)

    if binariesToRun == []:
        print("Warning: no testcases to run!")
        return

    [patchelf(binary, simics_glibc_path, interpreter, rpath, unwinder) for binary in binariesToRun]
    [patchelf(binary+ ".bad", simics_glibc_path, interpreter, rpath, unwinder) for binary in binariesToRun]
    print("Patched " + str(len(binariesToRun)) + "x2 binaries")

    list_of_results = runBinariesInDir(binariesToRun, simics_script_path, simics_env)
    print(list_of_results)
    print("Total testcases:%d, C3 detected:%d" %(total_testcases, c3_detected_testcases))

    exportResultInCSV(list_of_results, (os.path.expanduser('~') + '/cc_auto_results.csv'))
    print("Found " + str(len(binariesToRun)) + " binaries")
    print("Ran and saved " + str(len(list_of_results)) + " results to 'cc_auto_results.csv'")


if __name__== "__main__":
    main()
