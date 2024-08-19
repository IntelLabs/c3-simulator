# Copyright 2016-2024 Intel Corporation
# SPDX-License-Identifier: MIT

from __future__ import print_function
from simics import *
from re import search
import re
import subprocess
import tempfile

def cmd_to_shell(cmd):
    run_command("echo \""+ cmd +"\"")
    return os.system(cmd)

def strip_anci(string):
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', string)

def check_vars_defined(vars):
    bad = False
    for v in vars.split():
        if not v in simenv:
            bad = True
            print("Error: variable " + v + " not defined.")
        elif simenv[v] == "":
            bad = True
            print("Error: variable " + v + " is empty.")
        elif simenv[v] == None:
            bad = True
            print("Error: variable " + v + " is None.")
    run_command("exit")

def command_to_console(cmd, nofail=False):
    # Fixes at least some crashes with missed return values
    if len(cmd) == 62:
        cmd = cmd + " "
    run_command("$con.input \"" + cmd + "\n\"")
    run_command("bp.console_string.wait-for $con \"$ \"")

    # Test for zero return code
    run_command("$con.record-start")
    run_command("$con.input \"echo $?\n\" ") # Echo the return code of the last command
    run_command("bp.console_string.wait-for $con \"$ \"")
    res = int(strip_anci(run_command("$con.record-stop")).splitlines()[-2]) # Get output, Remove ANCI colorcodes, get second line, format as Integer.
    if res != 0 and not nofail:
        if simenv.exit == 1:
            print("Console command \"" + cmd + "\" returned code " + str(res) + ".\nTerminating simulation (due exit=1).")
            sys.exit(2)
        raise Exception("Console command \"" + cmd + "\" returned code " + str(res) + ". Script halted.")
    return res

def is_ubuntu():
    res = command_to_console("[[ $(lsb_release -si) == 'Ubuntu' ]]", True)
    return res == 0

# Preserves permissions, as opposed to matic0.upload-dir -follow
def upload_tarball(path):
    print("uload_tarball({})".format(path))
    res = os.system("tar -chzf temp.tar " + path)
    if res != 0:
        sys.exit(2)
    run_command("matic0.upload \"temp.tar\" \"/home/simics\"")
    run_command("matic0.wait-for-job")
    os.system("rm temp.tar")
    command_to_console("tar -xf /home/simics/temp.tar -C ~")


def rlinput(prompt, suggestion=''):
    res = input("%s [%s]:" % (prompt, suggestion))
    return res or suggestion

def getLatestCkptFromDir(wd="/opt/simics/checkpoints/"):
    dirs = next(os.walk(wd))[1] # Get list of directories in working directory
    dirs = [fl for fl in dirs if ".ckpt" in fl] # Filter only checkpoints
    dirs.sort(key=lambda e: os.stat(wd + e).st_ctime) # Sort by date created
    return wd + dirs[-1] # Get most recent

def getCkpt():
    matchingArgs = [arg for arg in sys.argv if (".ckpt" in arg)]
    if len(matchingArgs) > 0:
        config = matchingArgs[0]
        print("Using checkpoint " + config)
    else:
        config = rlinput("What configuration file do you want to use?", getLatestCkptFromDir())
    return config

def synchronizeGuestDate():
    out = subprocess.Popen(["LC_ALL=en.EN date"], stdout=subprocess.PIPE, shell=True).communicate()[0].decode("utf-8").rstrip()
    command_to_console("sudo date --set '" + out + "'")

def append_env_var(new_var):
    if hasattr(simenv, "env_vars") and simenv.env_vars != "":
        check = r"\b" + new_var + r"\b"
        if not search(check, simenv.env_vars):
            simenv.env_vars = f"{simenv.env_vars} {new_var}"
    else:
        simenv.env_vars = new_var

def copy_many_files(files, srcDir="", dstDir="/home/simics"):
    run_command("$con.input \"mkdir -p {}\\n\";".format(dstDir) +
                "bp.console_string.wait-for $con \"$ \"\n")
    rm_root = re.compile(r'^/')
    d = "\"" + dstDir + "\""
    for f in files.split():
        s = "\"" + srcDir + "/" + rm_root.sub('', f) + "\""
        # print("matic0.upload " + s + " " + d)
        run_command("matic0.upload " + s + " " + d)
    run_command("matic0.wait-for-job")

def copy_include_folders(folders):
    command_to_console("mkdir -p include")
    for path in folders.split(" "):
        print(f"transferring include folder {path}")

        cmd_to_shell(f"mkdir -p tmp")
        tarball_fn = f"{next(tempfile._get_candidate_names())}.tar"
        tarball_path = f"tmp/{tarball_fn}"
        parent_dir = os.path.dirname(path)
        dir = os.path.basename(path)

        # Python's os.path.basename differs from Linux's basename when the last
        # item in the path is a directory. On Linux, it returns the directory
        # itself; whereas on Python, the empty string. Feeding an empty string
        # to the tar command will result in an empty tarball (or an error
        # message complaining about the empty list of files to tar).
        if not dir:
            dir = '.'

        c_flag = ""
        if parent_dir:
            c_flag = f"-C {parent_dir}"

        res = cmd_to_shell(f"tar {c_flag} -chzf {tarball_path} {dir}")
        assert res == 0
        run_command(f"matic0.upload {tarball_path} /home/simics")
        run_command("matic0.wait-for-job")
        run_command(f"! rm -f '{tarball_path}'")
        command_to_console(f"tar -mxf ~/{tarball_fn} -C ~/include")

def assert_msg(test, msg):
    if not (test):
        print("!!!! Assertion failed:\n")
        print(msg)
        print("\nquitting...")
        sys.exit(3);
