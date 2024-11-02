# Copyright 2016-2024 Intel Corporation
# SPDX-License-Identifier: MIT

import simics
import instrumentation
import cli

# Called by <obj>.add-instrumentation framework prior to doing the
# actual connect to the provider.
def pre_connect(obj, provider, *tool_args):

    if not provider and any(tool_args):
        raise cli.CliError(
            "Error: tracer argument specified with nothing connected")

    (debug_on, break_on_decode_fault, disable_data_encryption) = tool_args

    # Without any specified flags, we enable everything by default.
    if not any(tool_args):
        debug_on = False
        break_on_decode_fault = False
        disable_data_encryption = False

    args = [["debug_on", debug_on],
            ["break_on_decode_fault", break_on_decode_fault],
            ["disable_data_encryption", disable_data_encryption]
    ]

    # Format a description based on the settings used.
    desc = ""
    desc += "Types:"
    desc += "D" if debug_on else ""
    desc += "I" if break_on_decode_fault else ""
    desc += "E" if disable_data_encryption else ""
    return (args, desc)

connect_args = [
    cli.arg(cli.flag_t, "-debug-on"),
    cli.arg(cli.flag_t, "-break-on-decode-fault"),
    cli.arg(cli.flag_t, "-disable-data-encryption"),
]

connect_doc = \
    """Each new connection to the tracer tool can be configured by
    supplying the following flags:

    <tt>-debug-on</tt> : Enable debug messages.
    <br/><tt>-break-on-decode-fault</tt> : Halt simulation when a non-canonical address is decoded
    <br/><tt>-disable-data-encryption</tt> : Disable C3 data encryption.

    If no flags are given, all flags will
    be disabled by default.!!"""


connect_extra_args = (connect_args, pre_connect, connect_doc)

instrumentation.make_tool_commands(
    "riscv_c3_model",
    object_prefix = "riscv_c3",
    provider_requirements = "cpu_instrumentation_subscribe",
    provider_names = ("processor", "processors"),
    connect_extra_args = connect_extra_args,
    new_cmd_doc = """TBD001.

    The <arg>file</arg> argument specifies a file
    to write the trace to, without any file, the trace will be printed
    to standard out.""")
