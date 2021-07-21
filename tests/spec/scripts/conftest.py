import pytest
import subprocess
import os

def pytest_addoption(parser):
    parser.addoption("--spec_year", action="store", required=True, choices=['06', '17'], help="Must specify SPEC year with '--spec_year <06|17>'")
