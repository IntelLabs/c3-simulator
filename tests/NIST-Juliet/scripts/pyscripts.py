from datetime import datetime

def nist_juliet_upload_and_run():
    
    run_command("read-configuration checkpoints/glibc_latest.ckpt")
    # set up variables, since the checkpoint does not preserve them
    run_command("$system = \"board\"")

    # Pick up the name of the serial console
    run_command("$con = $system.serconsole.con")
    run_command("$cpu = $system.mb.cpu0.core[0][0]")

    # run for one/tenth second to give the hardware time to update
    run_command("$cpu.wait-for-time 0.1 -relative")

    ###############################
    #     Upload Source files     #
    ###############################
    #$con.input "simics-agent \n"
    #$cpu.wait-for-time 0.1 -relative
    run_command("start-agent-manager")
    run_command("agent_manager.connect-to-agent")
    
    upload_tarball("tests/NIST-Juliet/")
    
    ###############################
    #     Compile & run           #
    ###############################
    command_to_console("ln -s /usr/lib64/libstdc++.so.6 /home/simics/glibc/glibc-2.30_install/lib")
    command_to_console("ln -s /usr/lib64/libgcc_s.so.1 /home/simics/glibc/glibc-2.30_install/lib")
    
    command_to_console("cd ~/tests/NIST-Juliet/testcases/CWE121_Stack_Based_Buffer_Overflow")
    
    command_to_console("./../../scripts/cc_nistjuliet_auto.py patchelf")
    
    #command_to_console("./../../scripts/cc_nistjuliet_auto.py runCweBinaries")
    #run_command("$cpu.wait-for-time 10 -relative")
    #run_command("matic0.download /home/simics/tests/NIST-Juliet/testcases/CWE121_Stack_Based_Buffer_Overflow/cc_auto_results.csv cc_auto_results_baseline_" + str(datetime.now().strftime("%H:%M:%S")) + ".csv")
    #run_command("matic0.wait-for-job")
    
    run_command("ptime")
    command_to_console("./../../scripts/cc_nistjuliet_auto.py runCweBinaries")
    run_command("ptime")
    run_command("$cpu.wait-for-time 10 -relative")
    run_command("matic0.download /home/simics/tests/NIST-Juliet/testcases/CWE121_Stack_Based_Buffer_Overflow/cc_auto_results.csv cc_auto_results_" + str(datetime.now().strftime("%H:%M:%S")) + ".csv")
    run_command("matic0.wait-for-job")
    run_command("stop")
    #################################
