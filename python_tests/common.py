import subprocess
import os
import sys

default_simics_options = ["-batch-mode"]
default_other_args = []

class SimicsInstance:
    def __init__(self, script, checkpoint, model, other_args = default_other_args):
        self.script = [script]
        self.checkpoint = checkpoint
        self.model = model
        self.simics_options = default_simics_options
        self.other_args = other_args
        
    
    def run(self, request, debug=0, capture=True):
        run_cmd = ["./simics"] + self.simics_options + self.script + ["checkpoint=" + self.checkpoint] +  ["model=" + self.model] + self.other_args
        if request.config.getoption("--slurm"):
            run_cmd = ["srun"] + ["--qos=inter"] + ["--partition={}".format(
                    request.config.getoption("--slurm-partition")
                )] + run_cmd
        print("\nrunning command:\n" , *run_cmd)

        if capture:
            # Avoid Text=true because it fails in newline translation on 
            # some outputs from address sanitizer
            return subprocess.run(run_cmd, capture_output=True)

        proc = subprocess.run(run_cmd)
        return proc

    def dump_proc(self, proc):
        # Simple, stupid, manual converting newlines to avoid Test=true bug
        return f'\nargs: {" ".join(self.other_args)}\n' + \
            f'STDOUT\n{proc.stdout.decode("utf-8", errors="ignore")}\n' + \
            f'STDERR\n{proc.stderr.decode("utf-8", errors="ignore")}\n'

class LocalUnitTest:
    cxx = ["g++"]
    cxx_flags = ["-O3", "-Werror",
        "-I/opt/simics/simics-6/simics-latest/src/include",
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
        print("\nrunning command:\n" , *run_cmd)
        return subprocess.run(run_cmd)

    def run(self, request, debug=0):
        run_cmd = "./{}".format(self.bin)
        print("\nrunning command:\n" , *run_cmd)
        return subprocess.run(run_cmd)