# Copyright 2016-2024 Intel Corporation
# SPDX-License-Identifier: MIT

from __future__ import print_function
from simics import *
import os

USER_PROVIDED_LINUX_TARGET = "qsp-x86/user-provided-linux"

def is_truthy(val):
    return (val == True or val == "true" or val == "TRUE" or val == "True" or
            val == "1")

def is_non_empty_string(val):
    return (val != None and val != "")

def load_user_provided_linux():
    args = [
        "load-target",
        USER_PROVIDED_LINUX_TARGET,
        "machine:hardware:consoles:gfx_con:show=0"
    ]

    if is_non_empty_string(simenv.kernel_cmdline):
        args.append(f"machine:software:linux:cmdline=\"{simenv.kernel_cmdline}\"")
    if is_truthy(simenv.connect_real_network):
        args.append("network:service_node:connect_real_network=\"napt\"")
    # Check $kernel and resolve with lookup-file
    if is_non_empty_string(simenv.kernel):
        simenv.kernel = run_command(f"lookup-file \"{simenv.kernel}\"")
        args.append(f"machine:software:linux:kernel=\"{simenv.kernel}\"")
        if not os.path.exists(simenv.kernel):
            raise Exception(f"Kernel file {simenv.kernel} does not exist")
    # Check $virtio_rootfs and resolve with lookup-file
    if is_non_empty_string(simenv.virtio_rootfs):
        simenv.virtio_rootfs = run_command(f"lookup-file \"{simenv.virtio_rootfs}\"")
        args.append(f"machine:software:linux:virtio_rootfs=\"{simenv.virtio_rootfs}\"")
        if not os.path.exists(simenv.virtio_rootfs):
            raise Exception(f"Virtio rootfs file {simenv.virtio_rootfs} does not exist")
    # Check $bios and resolve with lookup-file
    if is_non_empty_string(simenv.bios):
        simenv.bios = run_command(f"lookup-file \"{simenv.bios}\"")
        args.append(f"machine:hardware:firmware:bios=\"{simenv.bios}\"")
        if not os.path.exists(simenv.bios):
            raise Exception(f"BIOS file {simenv.bios} does not exist")

    print(" ".join(args))
    run_command(" ".join(args))

def set_model_attr(model, var, val):
    print(f"{model}->{var}={val}")
    run_command(f"{model}->{var}={val}")

def configure_model_bool_attr(model, var, val):
    if val != None and val != "":
        if is_truthy(val):
            val = "TRUE"
        else:
            val = "FALSE"
        set_model_attr(model, var, val)

def enable_c3():
    args = [ f"new-{simenv.model}-model", "-connect-all" ]

    if simenv.disable_data_encryption:
        args.append("-disable-data-encryption")
        print('''\
            ---------------------------------------
            | !!! CC data encryption disabled !!! |
            ---------------------------------------\
        ''')

    if simenv.enable_integrity:
        args.append("-integrity")

    print(" ".join(args))
    simenv.cc = run_command(" ".join(args))
    simenv.cc = f"{simenv.cc}_0"

    if is_truthy(simenv.disable_ptrenc):
        print('''\
            ----------------------------------------
            | !!! CC pointer encoding disabled !!! |
            ----------------------------------------\
        ''')
        set_model_attr(simenv.cc, "cc_isa_ptrenc", "FALSE")

    configure_model_bool_attr(simenv.cc, "gsrip_enabled", simenv.enable_gsrip)
    configure_model_bool_attr(simenv.cc, "integrity_break_on_write_mismatch", simenv.integrity_break_on_write_mismatch)
    configure_model_bool_attr(simenv.cc, "integrity_break_on_read_mismatch", simenv.integrity_break_on_read_mismatch)
    configure_model_bool_attr(simenv.cc, "integrity_fault_on_write_mismatch", simenv.integrity_fault_on_write_mismatch)
    configure_model_bool_attr(simenv.cc, "integrity_fault_on_read_mismatch", simenv.integrity_fault_on_read_mismatch)
    configure_model_bool_attr(simenv.cc, "integrity_warn_on_read_mismatch", simenv.integrity_warn_on_read_mismatch)
    configure_model_bool_attr(simenv.cc, "break_on_exception", simenv.break_on_exception)
