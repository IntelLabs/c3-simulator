from __future__ import print_function
from simics import *
import re
import subprocess

def cmd_to_shell(cmd):
    run_command("echo \""+ cmd +"\"")
    os.system(cmd)

def strip_anci(string):
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', string)

def command_to_console(cmd):
    run_command("$con.input \"" + cmd + "\n\"")
    run_command("$con.wait-for-string \"$ \"")
    
    # Test for zero return code
    run_command("$con.record-start")
    run_command("$con.input \"echo $?\n\" ") # Echo the return code of the last command
    run_command("$con.wait-for-string \"$ \"")
    res = int(strip_anci(run_command("$con.record-stop")).splitlines()[-2]) # Get output, Remove ANCI colorcodes, get second line, format as Integer.
    if res != 0:
        if simenv.exit == 1:
            print("Console command \"" + cmd + "\" returned code " + str(res) + ".\nTerminating simulation (due exit=1).")
            sys.exit(2)
        raise Exception("Console command \"" + cmd + "\" returned code " + str(res) + ". Script halted.")


def upload_tarball(path):
    os.system("tar -chzf temp.tar " + path)
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

def sychronizeGuestDate():
    out = subprocess.Popen(["date"], stdout=subprocess.PIPE, shell=True).communicate()[0].decode("utf-8").rstrip()
    command_to_console("sudo date --set '" + out + "'")
