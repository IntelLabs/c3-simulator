# Copyright 2021-2024 Intel Corporation
# SPDX-License-Identifier: MIT

import pytest
import subprocess
import sys


def pytest_addoption(parser):
    parser.addoption("--c3-debug",
                     action="store_true",
                     default=False,
                     help="Enable extra C3-specific debug outputs")
    parser.addoption("--c3-disable-data-encryption",
                     action="store_true",
                     default=False,
                     help="Disable data encryption in C3 model")
    parser.addoption(
        "--checkpoint",
        action="store",
        required=True,
        help=
        "Must specify path to Simics checkpoint with '--checkpoint path/to/checkpoint.ckpt'"
    )
    parser.addoption("--cxxflags", default=[], action="append")
    parser.addoption("--have-kernel",
                     action="store_true",
                     default=False,
                     help="Enable tests that require kernel support")
    parser.addoption("--libunwind",
                     action="store_true",
                     default=False,
                     help="Use LLVM libunwind")
    parser.addoption("--model", default=[], action="append")
    parser.addoption("--no-make",
                     action="store_true",
                     default=False,
                     help="Don't rebuild modules on start.")
    parser.addoption("--no-upload",
                     action="store_true",
                     default=False,
                     help="Don't upload any files to the system")
    parser.addoption("--nomodel", default=[], action="append")
    parser.addoption("--nospec", default=[], action="append")
    parser.addoption(
        "--rebuild",
        action="store_true",
        default=False,
        help="Build SPEC inside Simics instead of using pre-built binaries")
    parser.addoption("--integrity-fault-on-read", action="store_true", default=False, help="If integrity enabled, break on read")
    parser.addoption("--skip-slow",
                     action="store_true",
                     default=False,
                     help="May skip some slower unit tests (off by default)")
    parser.addoption(
        "--slurm",
        action="store_true",
        default=False,
        help=
        "Distribute workloads to SLURM cluster instead of running them locally"
    )
    parser.addoption("--slurm-partition",
                     action="store",
                     default="cpu-icx",
                     help="The SLURM partition to use")
    parser.addoption("--spec", default=[], action="append")
    parser.addoption("--spec_size", default=[], action="append")
    parser.addoption("--test-timeout",
                     action="store",
                     default=0,
                     help="Timeout for each test in seconds")
    parser.addoption("--upload-glibc",
                     action="store_true",
                     default=False,
                     help="Pass upload_glibc=1 to simics")

@pytest.fixture
def checkpoint(request):
    return request.config.getoption("--checkpoint")


@pytest.fixture
def rebuild(request):
    return request.config.getoption("--rebuild")

def ask_to_verify(msg):
    print(f"{msg}\nAre you sure you want to continue? (y/n)")
    if input().strip().lower() != "y":
        sys.exit(1)

def pytest_sessionstart(session):
    if session.config.getoption("--have-kernel") and session.config.getoption("--model"):
        models = session.config.getoption("--model")
        if "lim" in models:
            ask_to_verify("lim model is not supported with --have-kernel")
        if "lim_disp" in models:
            ask_to_verify("lim_disp model is not supported with --have-kernel")

    if not session.config.getoption("--no-make"):
        proc = subprocess.run("make")
        assert proc.returncode == 0
