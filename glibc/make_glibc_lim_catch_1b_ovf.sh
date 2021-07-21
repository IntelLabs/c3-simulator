#!/bin/bash

# Invoke this script when building glibc so that pointers to allocations are
# shifted such that even single-byte overflows will be detected using LIM.

CPPFLAGS="-DLIM_CATCH_1B_OVF" ./make_glibc.sh

