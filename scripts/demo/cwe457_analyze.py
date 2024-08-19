#!/usr/bin/python3
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("-v", "--verbose", type=int,
                    default=0,
                    help="Display verbose messages during processing")
parser.add_argument("-i", "--input", type=str,
                    default="cwe457.log",
                    help="Log file to process")
args = parser.parse_args()

def log(verbosity, text):
  if verbosity <= args.verbose:
    print(text, end="")

state = "BUILDLINE"
lastbuild = ""
with open(args.input, encoding="utf-8") as logfile:
  for line in logfile:
    if "Build command" in line:
      if state == "BUILDLINE":
        state = "RUNLINE"
      elif state == "UNINITLINE":
        print(f'*** no uninitialized read detected at:')
        print(f'    {lastbuild} ')
        state = "RUNLINE"
      else:
        print(f'*** failed to build:')
        print(f'    {lastbuild} ')
      log(1, line.rsplit(maxsplit=1)[-1]+"\n")
      lastbuild = line.rsplit(maxsplit=1)[-1]
    if "Running model" in line:
      if state == "RUNLINE":
        state = "UNINITLINE"
      else:
        raise RuntimeError
    if "Read from Uninitialized memory" in line:
      if state == "UNINITLINE" or state == "BUILDLINE":
        log(3, line)
        state = "BUILDLINE"
      else:
        raise RuntimeError
