#!/bin/bash

# invoke this script when building glibc for running memory usage experiments with LIM.

CPPFLAGS="-DLIM_NO_ENCODE -DLIM_TRC_FILE -DOPT_LIM_REALLOC" ./make_glibc.sh

