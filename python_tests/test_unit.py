import pytest
from python_tests.common import SimicsInstance

gtest_script 			= "unit_tests/runtest_common.simics"
generic_script          = "scripts/runworkload_common.simics"
src_path = "unit_tests/"
all_src_files = ["string_test.cpp", "fileio_test.cpp"]
default_models=["native", "lim"]

def pytest_generate_tests(metafunc):
    if "src_file" in metafunc.fixturenames:
        metafunc.parametrize("src_file", all_src_files)
    if "model" in metafunc.fixturenames:
        if metafunc.config.getoption("model"):
            metafunc.parametrize("model", metafunc.config.getoption("model"))
        else:
            metafunc.parametrize("model", default_models)

def test_rungtest(checkpoint, src_file, model):
    other_args = ["workload_name="+ src_file + ".o", "src_file="+ src_file, "src_path=" + src_path]
    test_inst = SimicsInstance(gtest_script, checkpoint, model, other_args)
    proc = test_inst.run()
    assert proc.returncode == 0

def test_lim_specific(checkpoint, model):
    if model != "lim":
        pytest.skip("Skipping")
    src_file = "lim_malloc_test.cpp"
    other_args = ["workload_name="+ src_file + ".o", "src_file="+ src_file, "src_path=" + src_path]
    model = "lim"
    test_inst = SimicsInstance(gtest_script, checkpoint, model, other_args)
    proc = test_inst.run()
    assert proc.returncode == 0
