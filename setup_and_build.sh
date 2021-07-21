#!/bin/sh

set -ex

if [ -z $SIMICS_BIN ]; then
    SIMICS_BIN=/opt/simics/simics-6.0.89/bin
fi

# Change to the directory containing the script:
cd `find $0 -printf "%h"`

# Setup the Simics project directory:
$SIMICS_BIN/project-setup --ignore-existing-files .
# Build the Simics modules:
make -B
# Download the patchelf archive used by some Simics scripts:
curl -o patchelf-0.10.tar.gz https://codeload.github.com/NixOS/patchelf/tar.gz/refs/tags/0.10
# Build glibc:
cd glibc
./make_glibc.sh
cd ..

# Upload glibc into Simics target and save a checkpoint
mkdir -p checkpoints
./simics -batch-mode scripts/make_glibc.simics buildmode=upload save_checkpoint=checkpoints/cc_glibc.ckpt
