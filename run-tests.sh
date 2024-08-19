#!/usr/bin/bash

# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

set -e

readonly c3_perf_url="https://github.com/IntelLabs/c3-perf-simulator"

if [ "${VERBOSE:=0}" -eq 1 ]; then
  VERBOSITY="-v"
fi

# By default, commands are run in quiet mode.
# To make them more verbose, set VERBOSITY to -v
VERBOSITY="${VERBOSITY:--q}"

# Set USE_ZIP_PKGS=1 if used with zipped repository packages.
# USE_ZIP_PKGS=1

if [ "${USE_ZIP_PKGS:=0}" -eq 1 ]; then
  sim_dir="c3-simulator"
else
  sim_dir="."
fi


# Check presence of simics files.
# If the first argument is "fail", terminate the script
_c3_simics_check () {
  SIMICSBASE=${sim_dir}/scripts/docker
  SIMICSISPM=intel-simics-package-manager-1.8.3-linux64.tar.gz
  SIMICSPKGS=simics-6-packages-2024-05-linux64.ispm
  echo "Checking presence of SIMICS installation files"
  if [ ! -d $SIMICSBASE ]
  then
    echo
    echo "Something went wrong with the directory structure"
    echo "(could not find $SIMICSBASE)"
    exit 1
  fi
  pushd $SIMICSBASE > /dev/null
  if [ ! -f $SIMICSISPM ] || [ ! -f $SIMICSPKGS ]
  then
    echo
    echo "ATTENTION!"
    echo
    echo "Please download the following SIMICS files into ${sim_dir}/scripts/docker:"
    echo
    echo "  $SIMICSISPM"
    echo "  $SIMICSPKGS"
    echo
    echo "These files can be found at:"
    echo "  https://lemcenter.intel.com/productDownload/?Product=256660e5-a404-4390-b436-f64324d94959"
    echo "  (or under Downloads at https://www.intel.com/content/www/us/en/developer/articles/tool/simics-simulator.html)"
    if [[ "$1" == "fail" ]]
    then
      exit 1
    fi
  fi
  popd > /dev/null
}

# Unzip the C3 simulator zip
# Get or extract the software stack (Linux, LLVM, Glibc, etc.)
# Also get the C3 Performance Simulator
_c3_unzip () {
  # Sanity check for zip files
  ARCHIVES=$(pwd)
  C3SIMZIP=c3-simulator-harden-may2024.zip
  C3LLVMZIP=c3-llvm-harden-may2024.zip
  for zip in $C3SIMZIP $C3LLVMZIP
  do
    if [ ! -f $ARCHIVES/$zip ]
    then
      echo
      echo "Could not find required zipped deliverable:"
      echo
      echo "    $zip"
      echo
      echo "Please download it from the Vault"
      echo
      echo "Your directory hierarchy should look like this:"
      echo "    ."
      echo "    |-- $C3SIMZIP"
      echo "    |-- $C3LLVMZIP"
      echo "    \`--- run-tests.sh"
      exit 1
    fi
  done

  # Extract C3 simulator zip
  echo "Unzipping C3 simulator"
  if [ ! -d ${sim_dir} ]
  then
    unzip $VERBOSITY $ARCHIVES/$C3SIMZIP
  else
    echo "... skipping (${sim_dir} directory already present)"
  fi

  # Extract LLVM zip
  LLVMSRC=${sim_dir}/llvm/src
  echo "Extracting LLVM into $LLVMSRC"
  if [ -z "$(ls -A $LLVMSRC)" ]
  then
    unzip $VERBOSITY $ARCHIVES/$C3LLVMZIP
    rmdir $LLVMSRC
    mv c3-llvm $LLVMSRC
  else
    echo "... skipping ($LLVMSRC directory already present)"
  fi

  # Get Linux from git repository
  LINUXURL=https://github.com/IntelLabs/c3-linux.git
  LINUXSRC=${sim_dir}/linux/src
  echo "Cloning Linux into $LINUXSRC"
  if [ -z "$(ls -A $LINUXSRC)" ]
  then
    git clone \
      $VERBOSITY \
      --config advice.detachedHead=false \
      --branch harden-may2024 \
      --depth 1 \
      $LINUXURL \
      $LINUXSRC
  else
    echo "... skipping ($LINUXSRC directory already present)"
  fi

  # Get Glibc from git repository
  GLIBCURL=https://github.com/IntelLabs/c3-glibc.git
  GLIBCSRC=${sim_dir}/glibc/src
  echo "Cloning Glibc into $GLIBCSRC"
  if [ -z "$(ls -A $GLIBCSRC)" ]
  then
    git clone \
      $VERBOSITY \
      --config advice.detachedHead=false \
      --branch harden-may2024 \
      --depth 1 \
      $GLIBCURL \
      $GLIBCSRC
  else
    echo "... skipping ($GLIBCSRC directory already present)"
  fi

  # Get EDK2 from git repositories
  #
  # Main repository
  EDK2MAINURL=https://github.com/IntelLabs/c3-edk2.git
  EDK2MAINSRC=${sim_dir}/edk2_src/edk2
  echo "Cloning EDK2 into $EDK2MAINSRC"
  if [ -z "$(ls -A $EDK2MAINSRC)" ]
  then
    git clone \
      $VERBOSITY \
      --config advice.detachedHead=false \
      --branch harden-may2024 \
      --depth 1 \
      --recurse-submodules \
      $EDK2MAINURL \
      $EDK2MAINSRC
  else
    echo "... skipping ($EDK2MAINSRC directory already present)"
  fi
  #
  # Platforms
  EDK2PLATFORMSURL=https://github.com/IntelLabs/c3-edk2-platforms.git
  EDK2PLATFORMSSRC=${sim_dir}/edk2_src/edk2-platforms
  echo "Cloning EDK2 Platforms into $EDK2PLATFORMSSRC"
  if [ -z "$(ls -A $EDK2PLATFORMSSRC)" ]
  then
    git clone \
      $VERBOSITY \
      --config advice.detachedHead=false \
      --branch harden-may2024 \
      --depth 1 \
      --recurse-submodules \
      $EDK2PLATFORMSURL \
      $EDK2PLATFORMSSRC
  else
    echo "... skipping ($EDK2PLATFORMSSRC directory already present)"
  fi
  #
  # Non-OSI modules
  EDK2NONOSIURL=https://github.com/tianocore/edk2-non-osi
  EDK2NONOSISRC=${sim_dir}/edk2_src/edk2-non-osi
  echo "Cloning EDK2 Non-OSI modules into $EDK2NONOSISRC"
  if [ -z "$(ls -A $EDK2NONOSISRC)" ]
  then
    git clone \
      $VERBOSITY \
      --config advice.detachedHead=false \
      --recurse-submodules \
      $EDK2NONOSIURL \
      $EDK2NONOSISRC
    pushd $EDK2NONOSISRC
    git checkout \
      41876073afb7c7309018223baa1a6f8108bf23f0
    popd
  else
    echo "... skipping ($EDK2NONOSISRC directory already present)"
  fi
  #
  # FSP
  EDK2FSPURL=https://github.com/intel/FSP
  EDK2FSPSRC=${sim_dir}/edk2_src/FSP
  echo "Cloning FSP into $EDK2FSPSRC"
  if [ -z "$(ls -A $EDK2FSPSRC)" ]
  then
    git clone \
      $VERBOSITY \
      --config advice.detachedHead=false \
      --recurse-submodules \
      $EDK2FSPURL \
      $EDK2FSPSRC
    pushd $EDK2FSPSRC
    git checkout \
      cf40b9ec6c7b0e4fc00ded7e121ea879cde94f6a
    popd
  else
    echo "... skipping ($EDK2FSPSRC directory already present)"
  fi

  # Get performance simulator from git repositories
  PERFSIMURL=https://github.com/IntelLabs/c3-perf-simulator
  PERFSIMSRC=c3-perf-simulator
  echo "Cloning Performance Simulator into $PERFSIMSRC"
  if [ ! -d $PERFSIMSRC ]
  then
    git clone \
      $VERBOSITY \
      --config advice.detachedHead=false \
      --branch harden-may2024 \
      --depth 1 \
      $PERFSIMURL \
      $PERFSIMSRC
  else
    echo "... skipping ($PERFSIMSRC directory already present)"
  fi

  # SIMICS installation files must be download by hand, thus call
  # _c3_simics_check to check that they are not present and print
  # instructions to download them.
  _c3_simics_check no-fail
}

# Build the full software stack and create SIMICS checkpoints
_c3_build () {
  # Sanity check for extracted files
  if [ ! -d ${sim_dir} ]
  then
    echo "${sim_dir} directory not found"
    echo "Did you run: $0 unzip ?"
    exit 1
  fi

  # Confirm that the user downloaded SIMICS installation files.
  # Fail it they have not.
  _c3_simics_check fail

  pushd ${sim_dir} > /dev/null

  # Our makefiles for docker use git ID information to name docker images and
  # checkpoints, thus start a git repository to provide the IDs.
  if (! git rev-parse &> /dev/null)
  then
    git init
    GIT_AUTHOR_NAME="name" \
    GIT_AUTHOR_EMAIL="name@domain" \
    GIT_COMMITTER_NAME="name" \
    GIT_COMMITTER_EMAIL="name@domain" \
    git commit --allow-empty -m "Initial commit"
  fi

  # Build the docker container
  make c3_docker

  # Create the base ubuntu checkpoint
  if ! make have_ubuntu_base_checkpoint
  then
    make c3_docker_ubuntu
  else
    echo "Skipping initial Ubuntu checkpoint creation, which might take hours."
    echo "To rebuild it, (re)move $(make have_ubuntu_base_checkpoint)"
  fi

  # Make a simics checkpoint with the software stack
  make c3_docker-ckpt-cc_kernel

  # Build the software stack
  make c3_docker-make_llvm

  popd > /dev/null
}

# Run the demos based on the first argument
_c3_demo () {
  # Sanity check for extracted files
  if [ ! -d ${sim_dir} ]
  then
    echo "${sim_dir} directory not found"
    echo "Did you run: $0 unzip ?"
    exit 1
  fi

  # Sanity check for built files
  if [ ! -e ${sim_dir}/checkpoints/cc_kernel.ckpt ]
  then
    echo "Kernel checkpoint not found (${sim_dir}/checkpoints/cc_kernel.ckpt)"
    echo "Did you run: $0 build ?"
    exit 1
  fi

  pushd ${sim_dir} > /dev/null

  # Select and run the demo
  if [[ "$1" == "1" ]] ||
     [[ "$1" == "cwe457" ]] ||
     [[ "$1" == "uninitialized_read" ]]
  then
    echo "Running the demo for Uninitialized Memory Read detection (CWE457)"
    make c3_docker-demo-cwe457.sh
  elif [[ "$1" == "2" ]] ||
       [[ "$1" == "clang_tidy" ]]
  then
    echo "Running the demo for Compiler Feedback (clang-tidy)"
    make c3_docker-demo-clang_tidy.sh
  elif [[ "$1" == "3" ]] ||
       [[ "$1" == "lldb" ]]
  then
    echo "Running the demo for LLDB"
    make c3_docker-demo-lldb_debug_01
  elif [[ "$1" == "5" ]] ||
       [[ "$1" == "edk" ]] ||
       [[ "$1" == "edk2" ]]
  then
    echo "Running the demo for EDK2"
    make edk2_all
    make c3_docker-edk2_run
  elif [[ "$1" == "6" ]] ||
       [[ "$1" == "kernel_heap" ]]
  then
    echo "Running the demo for Linux Kernel with Heap Protection"
    make c3_docker-linux_buildroot
  elif [[ "$1" == "7" ]] ||
       [[ "$1" == "juliet" ]]
  then
    echo "Running the demo for NIST Juliet"
    make c3_docker-demo-juliet-native
    make c3_docker-demo-juliet-native-stack
    make c3_docker-demo-juliet-c3-heap
    make c3_docker-demo-juliet-c3-heap-align
    make c3_docker-demo-juliet-c3-stack
  else
    echo "Invalid test name ($1)"
    exit 1
  fi

  popd > /dev/null
}

_c3_perf_demo () {
  if [ "${USE_ZIP_PKGS:=0}" -ne 1 ]; then
    echo "Not supported, please see ${c3_perf_url} to run."
    exit 1
  fi
  # Sanity check for extracted files
  if [ ! -d c3-perf-simulator ]
  then
    echo "c3-perf-simulator directory not found"
    echo "Did you run: $0 unzip ?"
    exit 1
  fi

  # Patch shell script for performance simulator demo
  DEMOSHELLFILE=c3-perf-simulator/build_docker.sh
  if [ ! -f $DEMOSHELLFILE ]
  then
    echo
    echo "Something went wrong with the directory structure"
    echo "(could not find $DEMOSHELLFILE)"
    exit 1
  fi
  if ! grep --quiet --extended-regexp "blowfish" $DEMOSHELLFILE
  then
    sed -i $DEMOSHELLFILE -e '/^docker run/s/$/ \\/'
    echo "    bash run_blowfish.sh" >> $DEMOSHELLFILE
  fi

  # Build the performance simulator
  pushd c3-perf-simulator > /dev/null

  ./build_docker.sh

  popd > /dev/null
}

# Print instructions and exit
_c3_usage () {
  echo "Usage:"
  if [ $USE_ZIP_PKGS -eq 1 ]; then
    echo "  $0 unzip"
  fi
  echo "  $0 build"
  echo "  $0 demo <arg>"
  exit 1
}

# Rudimentary argument parsing
if [ $# -eq 0 ]
then
  echo "Please provide an argument"
  _c3_usage
fi
if [[ "$1" == "unzip" ]]
then
  if [ "${USE_ZIP_PKGS:=0}" -eq 1 ]; then
    _c3_unzip
  else
    echo "Unzip option not available"
  fi
elif [[ "$1" == "build" ]]
then
  _c3_build
elif [[ "$1" == "demo" ]]
then
  if [ $# -le 1 ]
  then
    echo "The demo option requires an additional argument"
    echo "Options:"
    echo "  $0 demo uninitialized_read"
    echo "  $0 demo clang_tidy"
    echo "  $0 demo lldb"
    echo "  $0 demo stack_encryption"
    echo "  $0 demo edk2"
    echo "  $0 demo kernel_heap"
    echo "  $0 demo juliet"
    if [ "${USE_ZIP_PKGS:=0}" -eq 1 ]; then
      echo "  $0 demo performance_simulator"
    fi
    exit 1
  else
    if [[ "$2" == "perf" ]] ||
       [[ "$2" == "performance_simulator" ]]
    then
      echo "Running the Performance Simulator demo"
      _c3_perf_demo
    else
      _c3_demo $2
    fi
  fi
else
  echo "Unrecognized option"
  _c3_usage
fi

exit 0
