# Invoke this script from simics/internal/spec indirectly as ./scripts/run_all_on_local17.sh.

import pytest
import subprocess
import os

all_spec17 = ["blender17", "deepsjeng17", "gcc17", "imagick17", "lbm17", "leela17", "mcf17", "nab17", "namd17", "omnetpp17", "parest17", "perlbench17", "povray17", "x264_17", "xalancbmk17", "xz17"]

results_dir = "run_on_local_results17"

os.makedirs(results_dir, exist_ok=True)
os.environ['TRACE_ALLOCS'] = '1'

def pytest_generate_tests(metafunc):
    metafunc.parametrize("spec", all_spec17)

def test_runspec17(spec):
    run_cmd = "./scripts/run_on_native.sh " + spec
    print("\nrunning command:\n" , *run_cmd)
    #FNULL = open(os.devnull, 'w')
    #proc = subprocess.run(run_cmd, stdout=FNULL, stderr=subprocess.STDOUT)
    proc = subprocess.run(run_cmd, shell=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log_fname = results_dir + "/" + spec + ".log"
    print("\nwriting results to ", *log_fname)
    log_file = open(log_fname, 'w')
    log_file.write(proc.stdout)
    assert proc.returncode == 0
