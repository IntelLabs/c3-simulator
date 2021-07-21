import pytest
from python_tests.common import SimicsInstance

all_spec17 = ["blender17", "deepsjeng17", "gcc17", "imagick17", "lbm17", "leela17", "mcf17", "nab17", "namd17", "omnetpp17", "parest17", "perlbench17", "povray17", "x264_17", "xalancbmk17", "xz17"]

default_models=["native"]
default_spec_sizes=["test"]

def pytest_generate_tests(metafunc):
    if "spec" in metafunc.fixturenames:
        if metafunc.config.getoption("spec"):
            metafunc.parametrize("spec", metafunc.config.getoption("spec"))
        else:
            metafunc.parametrize("spec", all_spec17)
    if "spec_size" in metafunc.fixturenames:
        if metafunc.config.getoption("spec_size"):
            metafunc.parametrize("spec_size", metafunc.config.getoption("spec_size"))
        else:
            metafunc.parametrize("spec_size", default_spec_sizes)
    if "model" in metafunc.fixturenames:
        if metafunc.config.getoption("model"):
            metafunc.parametrize("model", metafunc.config.getoption("model"))
        else:
            metafunc.parametrize("model", default_models)

def test_runspec17(checkpoint, spec, spec_size, model):
    specscript = "tests/spec/scripts/generic.simics"
    #model="native"
    spec_args = ["spec_size=" + spec_size, "spec=" + spec]
    test_inst = SimicsInstance(specscript, checkpoint, model, spec_args)
    proc = test_inst.run()
    assert proc.returncode == 0

