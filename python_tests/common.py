import subprocess
import os

default_simics_options = ["-batch-mode"]
default_other_args = []

class SimicsInstance:
    def __init__(self, script, checkpoint, model, other_args = default_other_args):
        self.script = [script]
        self.checkpoint = checkpoint
        self.model = model
        self.simics_options = default_simics_options
        self.other_args = other_args
        
    
    def run(self, debug=0):
        run_cmd = ["./simics"] + self.simics_options + self.script + ["checkpoint=" + self.checkpoint] +  ["model=" + self.model] + self.other_args
        print("\nrunning command:\n" , *run_cmd)
        proc = subprocess.run(run_cmd)
        return proc
