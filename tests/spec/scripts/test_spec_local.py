# Invoke this script from simics/internal/spec indirectly as ./scripts/run_all_on_local[06|17].sh.

import pytest
import subprocess
import os

results_dir = ""

def pytest_generate_tests(metafunc):
    spec_year = metafunc.config.getoption("--spec_year", skip=True)

    all_spec = []
    if spec_year == "06":
        all_spec = ["astar", "bzip", "dealII", "gcc", "gobmk", "h264ref", "hmmer", "lbm", "libquantum", "mcf", "milc", "namd", "omnetpp", "perlbench", "povray", "sjeng", "soplex", "sphinx", "xalancbmk"]
    elif spec_year == "17":
        all_spec = ["blender17", "deepsjeng17", "gcc17", "imagick17", "lbm17", "leela17", "mcf17", "nab17", "namd17", "omnetpp17", "parest17", "perlbench17", "povray17", "x264_17", "xalancbmk17", "xz17"]
    else:
        print('Invalid SPEC year: ' + spec_year)
        exit

    global results_dir
    results_dir = "run_on_local_results" + spec_year

    os.makedirs(results_dir, exist_ok=True)
    os.environ['TRACE_ALLOCS'] = '1'

    metafunc.parametrize("spec", all_spec)

def test_runspec(spec):
    run_cmd = "./scripts/run_on_native.sh " + spec
    print("\nrunning command:\n" , *run_cmd)
    #FNULL = open(os.devnull, 'w')
    #proc = subprocess.run(run_cmd, stdout=FNULL, stderr=subprocess.STDOUT)
    proc = subprocess.run(run_cmd, shell=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    global results_dir
    log_fname = results_dir + "/" + spec + ".log"
    print("\nwriting results to ", *log_fname)
    log_file = open(log_fname, 'w')
    log_file.write(proc.stdout)
    assert proc.returncode == 0
