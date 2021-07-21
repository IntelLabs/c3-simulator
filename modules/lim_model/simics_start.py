# Copyright 2016 Intel Corporation
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

    (debug_on, break_on_decode_fault, 
     disable_meta_check, break_on_exception, trace_only) = tool_args

    # Without any specified flags, we enable everything by default. 
    if not any(tool_args):
        debug_on = False
        break_on_decode_fault = False
        disable_meta_check = False
        break_on_exception = False
        trace_only = False


    args = [["debug_on", debug_on],
            ["break_on_decode_fault", break_on_decode_fault],
            ["disable_meta_check", disable_meta_check],
            ["break_on_exception", break_on_exception],
            ["trace_only", trace_only]]

    # Format a description based on the settings used.
    desc = ""    
    desc += "D" if debug_on else ""
    desc += "I" if break_on_decode_fault else ""
    desc += " disable-meta-check" if disable_meta_check else ""
    desc += "V" if break_on_exception else ""
    desc += "P" if trace_only else ""
    
    return (args, desc)

connect_args = [
    cli.arg(cli.flag_t, "-debug-on"),
    cli.arg(cli.flag_t, "-break-on-decode-fault"),
    cli.arg(cli.flag_t, "-disable-meta-check"),
    cli.arg(cli.flag_t, "-break-on-exception"),
    cli.arg(cli.flag_t, "-trace-only")]

connect_doc = \
    """Each new connection to the tracer tool can be configured by
    supplying the following flags:
    
    <tt>-debug-on</tt> : Enable debug messages.
    <br/><tt>-break-on-decode-fault</tt> : Halt simulation when a non-canonical address is decoded
    <br/><tt>-disable-meta-check</tt> : Disable LIM tag and bounds check.

    If no flags are given, all flags will
    be disabled by default.!!"""


def new_command_fn(tool_class, name, filename):
    return simics.SIM_create_object(tool_class, name, [["file", filename]])

connect_extra_args = (connect_args, pre_connect, connect_doc)
new_cmd_extra_args = ([cli.arg(cli.filename_t(), "file", "?")], new_command_fn)

instrumentation.make_tool_commands(
    "lim_model",
    object_prefix = "lim",
    provider_requirements = "cpu_instrumentation_subscribe",    
    provider_names = ("processor", "processors"),
    connect_extra_args = connect_extra_args,
    new_cmd_extra_args = new_cmd_extra_args,
    new_cmd_doc = """
	""")
