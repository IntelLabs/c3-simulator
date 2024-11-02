# Copyright 2021-2024 Intel Corporation
# SPDX-License-Identifier: MIT

import subprocess
import os
import sys

default_simics_options = ["-batch-mode"]
default_other_args = []

default_models = ["native"]
heap_models = [
    "c3", "c3-integrity", "c3-integrity-intra", "c3-castack", "c3-nowrap"
]
stack_models = ["c3-castack"]
lim_models = ["lim", "lim_disp", "lim-trace"]



default_models.extend(heap_models)
default_models.extend(stack_models)
default_models.extend(lim_models)

default_models = list(set(default_models))
default_models.sort()  # need to make sure list is consistent between runs
heap_models = list(set(heap_models))
heap_models.sort()  # need to make sure list is consistent between runs
stack_models = list(set(stack_models))
stack_models.sort()  # need to make sure list is consistent between runs
lim_models = list(set(lim_models))
lim_models.sort()  # need to make sure list is consistent between runs

def add_if_not_present(lst, item):
    if not item in lst:
        lst.append(item)

def module_dir(model):
    base_model = model.split("-", 1)[0]
    return "modules/{}_model".format(base_model)

def get_model_match_list(model):
    match_list = ["*", model]
    if model in heap_models:
        match_list.append("cc")
    # #ifdef CC_ZTS_ENABLE
    if model.endswith("-zts"):
        match_list.append("zts")
    # #endif  // CC_ZTS_ENABLE
    if model in lim_models:
        match_list.append("lim")
    return match_list

def get_active_models(metafunc, def_models = default_models):
    models = []

    if metafunc.config.getoption("model"):
        # use models from options if provided
        models.extend(metafunc.config.getoption("model"))
    else:
        # Otherwise check the modules folders for any of the default_models
        models = filter(lambda m: os.path.exists(module_dir(m)), def_models)
        # Remove lim models if the custom kernel is enabled
        if metafunc.config.getoption("--have-kernel"):
            models = filter(lambda m: not m in ["lim", "lim_disp"], models)

    # Filter out models in nomodel
    if metafunc.config.getoption("nomodel"):
        no_models = metafunc.config.getoption("nomodel")
        # Keep only models that cannot be matched with the nomodel list
        models = filter(
            lambda m: not len(set(get_model_match_list(m)).intersection(set(no_models))),
            models)

    return models


def add_common_simics_args(args, request, model):


    if request.config.getoption("--no-upload"):
        add_if_not_present(args, "no_upload=TRUE")

    if model.endswith("-castack"):
        add_if_not_present(args, "enable_cc_castack=TRUE")

    if model.endswith("-integrity"):
        add_if_not_present(args, "enable_integrity=TRUE")
        add_if_not_present(args, "integrity_fault_on_write_mismatch=TRUE")
        # Disable breakpoints as they will silently and erroneously succeed
        # when ran in --batch-mode as unit_tests are typically ran.
        add_if_not_present(args, "integrity_break_on_read_mismatch=FALSE")
        add_if_not_present(args, "integrity_break_on_write_mismatch=FALSE")
        # Enable read mismatch faults if enabled
        if request.config.getoption("--integrity-fault-on-read"):
            add_if_not_present(args, "integrity_fault_on_read_mismatch=TRUE")
        else:
            add_if_not_present(args, "integrity_fault_on_read_mismatch=FALSE")

    if model.endswith("-integrity-intra"):
        add_if_not_present(args, "enable_integrity=TRUE")
        add_if_not_present(args, "compiler=/home/simics/llvm/llvm_install/bin/clang++")

    if model.endswith("-nowrap"):
        add_if_not_present(args, "enable_cc_nowrap=TRUE")
        add_if_not_present(args, "disable_cc_env=TRUE")

    if model == "lim-trace":
        add_if_not_present(args, "trace_only=TRUE")

    if request.config.getoption("--libunwind"):
        add_if_not_present(args, "unwinder=llvm_libunwind")
    else:
        add_if_not_present(args, "unwinder=default_unwinder")

    return args

def cleanup_simics_model_name(model):
    if model.endswith("-integrity"):
        return model.replace('-integrity', '')

    if model.endswith("-integrity-intra"):
        return model.replace('-integrity-intra', '')



    if model.endswith("-castack"):
        return model.replace('-castack', '')

    if model.endswith("-nowrap"):
        return model.replace('-nowrap', '')

    if model == "lim-trace":
        return "lim_disp"

    return model

class SimicsInstance:

    def __init__(self,
                 script,
                 checkpoint,
                 model,
                 other_args=default_other_args):
        self.script = [script]
        self.checkpoint = checkpoint
        self.model = model
        self.simics_options = default_simics_options
        self.other_args = other_args

    def add_to_cmd(self, run_cmd, opt):
        if not opt in run_cmd:
            run_cmd.append(opt)
        return run_cmd

    def run(self, request, debug=0, capture=True):
        run_cmd = ["./simics"] + self.simics_options + self.script + [
            "checkpoint=" + self.checkpoint
        ] + ["model=" + self.model] + self.other_args
        if request.config.getoption("--slurm"):
            run_cmd = ["srun"] + ["--qos=inter"] + [
                "--partition={}".format(
                    request.config.getoption("--slurm-partition"))
            ] + run_cmd
        print("\nrunning command:\n", *run_cmd)

        extra_run_args = {}

        if request.config.getoption("--c3-debug"):
            run_cmd = self.add_to_cmd(run_cmd, "c3_debug=TRUE")
        if request.config.getoption("--c3-disable-data-encryption"):
            run_cmd = self.add_to_cmd(run_cmd, "disable_data_encryption=TRUE")
        if request.config.getoption("--upload-glibc"):
            run_cmd = self.add_to_cmd(run_cmd, "upload_glibc=TRUE")
        if request.config.getoption("--test-timeout") != 0:
            extra_run_args["timeout"] = int(
                request.config.getoption("--test-timeout"))

        if request.config.getoption("--c3-debug"):
            extra_run_args["stdout"] = sys.stdin
            extra_run_args["stdin"] = sys.stdin
            extra_run_args["stderr"] = sys.stderr
        elif capture:
            # Avoid Text=true because it fails in newline translation on
            # some outputs from address sanitizer
            extra_run_args["capture_output"] = True

        proc = subprocess.run(run_cmd, **extra_run_args)
        return proc

    def decode_proc_ostream(self, stream):
        return (stream.decode("utf-8", errors="ignore").replace(
            "\\r\\n", "\r").replace("\\n", "\n"))

    def dump_proc(self, proc):
        return f'\nargs: {" ".join(self.other_args)}\n' + \
            f'STDOUT\n{self.decode_proc_ostream(proc.stdout)}\n' + \
            f'STDERR\n{self.decode_proc_ostream(proc.stderr)}\n'


class LocalUnitTest:
    cxx = ["g++"]
    cxx_flags = [
        "-O3", "-Werror", "-I/opt/simics/simics-6/simics-latest/src/include",
        "-pthread"
    ]
    ld_flags = ["-lgtest", "-lgtest_main"]

    def __init__(self, bin, *srcs):
        self.bin = os.path.join("unit_tests", bin)
        self.srcs = []
        for f in srcs:
            f = os.path.join("unit_tests", f)
            self.srcs.append(f)

    def build(self, request, debug=0):
        for f in self.srcs:
            if not os.path.isfile(f):
                print("Cannot find {}".format(f), file=sys.stderr)
                return -1

        run_cmd = self.cxx + self.cxx_flags + self.srcs + \
                  self.ld_flags + ["-o"] + [self.bin]
        print("\nrunning command:\n", *run_cmd)
        return subprocess.run(run_cmd)

    def run(self, request, debug=0):
        run_cmd = "./{}".format(self.bin)
        print("\nrunning command:\n", *run_cmd)
        return subprocess.run(run_cmd)
