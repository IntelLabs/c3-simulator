# Copyright 2021-2024 Intel Corporation
# SPDX-License-Identifier: MIT

import pytest
import subprocess
import os

def pytest_addoption(parser):
    parser.addoption("--checkpoint", action="store", required=True, help="Must specify path to Simics checkpoint with '--checkpoint path/to/checkpoint.ckpt'")
    parser.addoption("--cxxflags", default = [], action="append")
    parser.addoption("--have-kernel", action="store_true", default=False, help="Enable tests that require kernel support")
    parser.addoption("--libunwind", action="store_true", default=False, help="Use LLVM libunwind")
    parser.addoption("--model", default = [], action="append")
    parser.addoption("--no-make", action="store_true", default=False, help="Don't rebuild modules on start.")
    parser.addoption("--no-upload", action="store_true", default=False, help="Don't upload any files to the system")
    parser.addoption("--nomodel", default = [], action="append")
    parser.addoption("--nospec", default = [], action="append")
    parser.addoption("--rebuild", action="store_true", default=False, help="Build SPEC inside Simics instead of using pre-built binaries")
    parser.addoption("--skip-slow", action="store_true", default=False, help="May skip some slower unit tests (off by default)")
    parser.addoption("--slurm", action="store_true", default=False, help="Distribute workloads to SLURM cluster instead of running them locally")
    parser.addoption("--slurm-partition", action="store", default="cpu-icx", help="The SLURM partition to use")
    parser.addoption("--spec", default = [], action="append")
    parser.addoption("--spec_size", default = [], action="append")
    parser.addoption("--upload-glibc", action="store_true", default=False, help="Pass upload_glibc=1 to simics")

@pytest.fixture
def checkpoint(request):
    return request.config.getoption("--checkpoint")

@pytest.fixture
def rebuild(request):
    return request.config.getoption("--rebuild")

def pytest_sessionstart(session):
    if session.config.getoption("--no-make"):
        return
    proc = subprocess.run("make")
    assert proc.returncode == 0
