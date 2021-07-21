import pytest
import subprocess
import os

def pytest_addoption(parser):
    parser.addoption("--checkpoint", action="store", required=True, help="Must specify path to Simics checkpoint with '--checkpoint path/to/checkpoint.ckpt'")
    parser.addoption("--spec", default = [], action="append")
    parser.addoption("--model", default = [], action="append")
    parser.addoption("--spec_size", default = [], action="append")

@pytest.fixture
def checkpoint(request):
    return request.config.getoption("--checkpoint")

def pytest_sessionstart(session):
    proc = subprocess.run("make")
    assert proc.returncode == 0
