import pytest
import os
import re
import sys
from python_tests.common import SimicsInstance

DEBUG_TEST_RUNNER = False

TEST_HEADER_TRUE_VALUES = [ "yes", "Yes", "true", 1 ]

""" Test header labels

The test header labels are put into the test source files themselves and will
here be used to configure and select tests to run.

model
    Run only for listed (space separated) modules
nomodel
    Run for all models, except the listed ones
need_libunwind
    Need support for C3-aware libunwind
need_kernel
    Set to yes to run only for tests that have custom kernel
no_kernel
    Set to yes to disable for tests that have custom kernel.
should_fail
    Negative test (e.g., attack), should fail or crash
envp
    Configure additional environment variables
ld_flags
    Configure additional LD_FLAGS
cxx_flags
    Configure additional CXX_FLAGS
simics_args
    Additional Simics options needed for test
"""
TEST_HEADER_LABELS = ["envp", "ld_flags", "should_fail", "cxx_flags", "model",
                      "nomodel", "xfail", "need_libunwind", "need_kernel",
                      "no_kernel", "simics_args"]

gtest_script 			= "unit_tests/runtest_common.simics"
generic_script          = "scripts/runworkload_common.simics"
src_path = "unit_tests/"

default_models=[ "native" ]

heap_models=[ "c3", "c3-integrity", "c3-integrity-intra" ]




default_models.extend(heap_models)




lim_models=[ "lim" ]
lim_models.extend([ "lim_disp", "lim-trace" ])
default_models.extend(lim_models)

slow_tests = [
    "malloc_test.cpp",
    "wcstring_test.cpp",
    "wcstring_test1.cpp",
    "wcstring_test2.cpp",
    "string_test1.cpp",
    "string_test2.cpp",
    "string_test3.cpp",
    "string_test4.cpp"
]

def dbgprint(*args):
    if DEBUG_TEST_RUNNER:
        print(*args, file=sys.stderr)

class c3_test_case:
    def __init__(self, filename):
        self.filename = filename
        self.header = {}

    def __str__(self):
        return self.filename

    def get_basename(self):
        return os.path.basename(self.filename)

    def get_dirname(self):
        return os.path.dirname(self.filename)

    def read_label(self, line, label):
        m = re.match(r"//\s+" + re.escape(label) + r":\s*(.*)$", line)
        if m:
            return m.group(1)
        return None

    def get_split(self, label, separator):
        val = self.get(label)
        if not val:
            return []
        return val.split(separator)

    def get(self, label):
        assert label in TEST_HEADER_LABELS, f"Unknown label: {label}"
        if not label in self.header:
            return None
        return self.header[label]

    def get_and_append(self, label, str):
        assert label in TEST_HEADER_LABELS, f"Unknown label: {label}"
        if not label in self.header:
            return str
        return f"{str} {self.header[label]}"

    def get_bool(self, label, default):
        assert label in TEST_HEADER_LABELS, f"Unknown label: {label}"
        if not label in self.header:
            return default
        if self.header[label] in TEST_HEADER_TRUE_VALUES:
            return True
        return False

    def has_model(self, label, model):
        assert label in TEST_HEADER_LABELS, f"Unknown label: {label}"
        dbgprint(f"=== ==== Looking for {model} in {label}")
        if not label in self.header:
            return False
        str = self.get(label)

        dbgprint(f"=== ==== {label} value is: {str}")

        r = (r".*[\s|,]?\s*" + re.escape(model) + r"\s*(?:[\s|,].*)?$")
        if re.match(r, str):
            return True

        r = (r".*[\s|,]?\s*" + re.escape(model.replace("-integrity-intra", "")) + r"\s*(?:[\s|,].*)?$")
        if re.match(r, str):
            return True

        r = (r".*[\s|,]?\s*" + re.escape(model.replace("-integrity", "")) + r"\s*(?:[\s|,].*)?$")
        if re.match(r, str):
            return True

        # Also match stuff like nomodel: -integrity
        model_split = model.split('-')
        if (len(model_split) > 1):
            submodel = model_split[1]
            r = (r".*[\s|,]?\s*" + re.escape(submodel) + r"\s*(?:[\s|,].*)?$")
            if re.match(r, str):
                return True
            if (len(model_split) > 2):
                submodel = f"{submodel}-{model_split[2]}"
                r = (r".*[\s|,]?\s*" + re.escape(submodel) + r"\s*(?:[\s|,].*)?$")
                if re.match(r, str):
                    return True

        return False

    def read_test_header(self):
        dbgprint(f"Called this one")
        f = open(self.filename, "r")

        found_header = False
        for line in f.readlines():
            line = line.rstrip()
            dbgprint(f"=== Reading: {line}")

            # Skip unti we find first consecutive block of // comments, and
            # stop reading after block ends
            if re.match("^//", line):
                dbgprint("=== >>> Found header section start")
                found_header = True
            elif found_header:
                dbgprint("=== <<< Found header section end")
                break
            elif not found_header:
                continue

            # Then check for recognized file headers
            found = False
            for label in TEST_HEADER_LABELS:
                found = self.read_label(line, label)
                dbgprint(f"=== Checking for {label}")
                if found != None:
                    dbgprint(f"=== Checking for {label}")
                    dbgprint(f"===              {label} <- {found}")
                    self.header[label] = found
                    break
            if not found:
                dbgprint(f"Unrecognized line: {line}")

        f.close()


def skip_slow(request):
    return request.config.getoption("--skip-slow")

def has_libunwind(request):
    return not request.config.getoption("--zts-default-unwinder")

def has_kernel(request):
    return request.config.getoption("--have-kernel")

def get_unwind_args(request):
    if has_libunwind(request):
        return [ "unwinder=llvm_libunwind" ]
    return [ "unwinder=default_unwinder" ]

def get_model_class(model):
    if model in heap_models:
        return "cc"


    if model in lim_models:
        return "lim"
    return model

def get_include_file(src_file):
    assert src_file.endswith(".cpp")
    files = [ ]

    if os.path.exists(os.path.join(os.path.dirname(src_file), "common.h")):
        files.append("common.h")

    if src_file.endswith("-fp.cpp"):
        nofp_file = os.path.basename(src_file.replace("-fp.cpp", ".cpp"))
        if os.path.exists(os.path.join(os.path.dirname(src_file), nofp_file)):
            files.append(nofp_file)

    return "include_file=\"" + " ".join(files) + "\""

def get_obj_files(request, testcase, model):
    """Get obj_files Simics arg"""
    obj_files = os.path.basename(testcase.filename)

    if has_kernel(request):
        obj_files += " /usr/lib/x86_64-linux-gnu/libgtest.a"
    return obj_files

def get_env_vars(request, testcase, model):
    return testcase.get_and_append("envp",
        "LD_LIBRARY_PATH=/home/simics/glibc/glibc-2.30_install/lib")

def get_ld_flags(request, testcase, model):
    return testcase.get_and_append("ld_flags", "")

def get_should_fail(request, testcase, model):
    return testcase.get_and_append("should_fail", "")

def get_cxx_flags(request, testcase, model):
    cxx_flags = testcase.get_and_append("cxx_flags", " ".join([
            "-DC3_MODEL={}".format(model),
            "-Iinclude" ]))
    if has_libunwind(request):
        cxx_flags = " ".join([ cxx_flags, "-ldl -pthread" ])

    # add cxx flags for the intra-obj model
    # only if they were not already specified in testcase headers
    if model == "cc-integrity-intra":
        if not "-fuse-ld=lld" in cxx_flags:
            cxx_flags = " ".join([ cxx_flags, "-fuse-ld=lld" ])
        # NOTE: this checks for the generic argument, allowing
        # the test header to take priority if the mode is relevant
        # e.g., -finsert-intraobject-tripwires={all,attr,none}
        if not "-finsert-intraobject-tripwires" in cxx_flags:
            cxx_flags = " ".join([ cxx_flags, "-finsert-intraobject-tripwires=all" ])

    return cxx_flags

def do_skip(request, testcase, model):
    """Check test file too see if it matches current run, otherwise skip"""

    found_model = False
    for l in [ "*", model, get_model_class(model) ]:
        if testcase.has_model("model", l):
            found_model = True
    if not found_model:
        pytest.skip(f"skippint, test not enabled for model")
        return True

    # If --skip-slow, then skip tests listed in slow_test
    if skip_slow(request):
        if any(t for t in slow_tests if t == os.path.basename(testcase.filename)):
            pytest.skip(f"skipping, slow test")
            return True

    # Skip if need but don't have kernel
    if not has_kernel(request) and testcase.get_bool("need_kernel", False):
        pytest.skip("sipping, requires kernel support")
        return True

    # Skip if have but cannot run with custom kernel
    if has_kernel(request) and testcase.get_bool("no_kernel", False):
        pytest.skip("sipping, not compatible with kernel support")
        return True

    # Skip if need but don't have libunwind
    if not has_libunwind(request) and testcase.get_bool("need_libunwind", False):
        pytest.skip("skipping, requires libunwind")
        return True

    # Skip if explicitly excluded
    for l in [ "*", model, get_model_class(model) ]:
        if testcase.has_model("nomodel", l):
            pytest.skip("skipping, test explicitly disabled")
            return True

    # Intra-object tests currently require kernel checkpoint
    if model.endswith("-integrity-intra") and not has_kernel(request):
        pytest.skip("skipping, intra-object tests on non C3-kernel")
        return True

    return False

def do_xfail(testcase, model):
    for l in [ "*", model, get_model_class(model) ]:
        if testcase.has_model("xfail", l):
            pytest.xfail("marked as xfail for {model}")
            return True
    return False

def module_dir(model):
    base_model = model.split("-", 1)[0]
    return "modules/{}_model".format(base_model)

def parametrize_model(metafunc):
    models = [ ]

    if metafunc.config.getoption("model"):
        # use models from options if provided
        models.extend(metafunc.config.getoption("model"))
    else:
        # Otherwise check the modules folders for any of the default_models
        models = filter(lambda m: os.path.exists(
            module_dir(m)), default_models)

    # Filter out models in nomodel
    if metafunc.config.getoption("nomodel"):
        nomodel = metafunc.config.getoption("nomodel")
        models = filter(
            lambda m: not (m in nomodel or get_model_class(m) in nomodel),
            models)

    metafunc.parametrize("model", models)

testfiles = {}

def pytest_generate_tests(metafunc):
    self_test()

    all_src_files = []

    # Scan through all the source files under src_path
    for root, dirs, files in os.walk(src_path):
        for file in files:
            if file.endswith(".cpp"):
                filename = os.path.join(root, file)
                testcase = c3_test_case(filename)
                testcase.read_test_header()
                # Just include files that include any model
                if testcase.get("model") != None:
                    all_src_files.append(testcase.filename)
                    testfiles[testcase.filename] = testcase

    if "src_file" in metafunc.fixturenames:
        metafunc.parametrize("src_file", all_src_files)
    if "model" in metafunc.fixturenames:
        parametrize_model(metafunc)

def test_rungtest(checkpoint, src_file, model, request):
    testcase = testfiles[src_file]
    # Do skip or xfail if needed
    if not do_skip(request, testcase, model):
        do_xfail(testcase, model)

    src_basename = testcase.get_basename()
    src_dirname = testcase.get_dirname()

    other_args = [
        "workload_name=" + src_basename + ".o",
        "src_file=" + src_basename,
        "src_path=" + src_dirname,
        "obj_files=\"" + get_obj_files(request, testcase, model) + "\"",
        "env_vars=\"" + get_env_vars(request, testcase, model) + "\"",
        "ld_flags=\"" + get_ld_flags(request, testcase, model) + "\"",
        "gcc_flags=\"" + get_cxx_flags(request, testcase, model) + "\"",
        "include_folders=\"unit_tests/include/unit_tests\"",
    ]

    other_args.extend(testcase.get_split("simics_args", " "))

    if has_kernel(request):
        other_args.append("zts_legacy_always_on=FALSE")
    else:
        other_args.append("zts_legacy_always_on=TRUE")




    if model.endswith("-integrity"):
        model = model.replace('-integrity', '')
        if not "enable_integrity=1" in other_args:
            other_args.append("enable_integrity=1")

    if model.endswith("-integrity-intra"):
        model = model.replace('-integrity-intra', '')
        if not "enable_integrity=1" in other_args:
            other_args.append("enable_integrity=1")

        # always add compiler
        other_args.append("compiler=/home/simics/llvm/llvm_install/bin/clang++")

    if model == "lim-trace":
        model = "lim_disp"
        other_args.append("trace_only=1")

    other_args.extend(get_unwind_args(request))

    other_args.append(get_include_file(testcase.filename))

    if request.config.getoption("--upload-glibc"):
        other_args.append("upload_glibc=1")

    test_inst = SimicsInstance(gtest_script, checkpoint, model, other_args)
    proc = test_inst.run(request)
    if (get_should_fail(request, testcase, model)):
        assert proc.returncode != 0, test_inst.dump_proc(proc)
    else:
        assert proc.returncode == 0, test_inst.dump_proc(proc)

def self_test():
    dbgprint("Running self_test")
    assert get_model_class("lim_disp") == "lim"
