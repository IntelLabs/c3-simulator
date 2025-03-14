# Experimental RISC-V C3

> **NOTE**: This is a very early proof-of-concept and lacks many of the features from the x86 C3 model and may have broken functionality.

> **NOTE**: Running via Docker is not fully working and may require additional changes. Particularly the commands that rely on SSH may need additional setup to forward connections via Docker into Simics.

To install, you need to make sure you're simics project (and Simics installation) has relevant Simics modules. Please check
<https://community.intel.com/t5/Blogs/Products-and-Solutions/Software/Introducing-RISC-V-in-the-Simics-Simulator/post/1496444>
for a general introduction on Simics RISC-V.

If you have the relevant Simics packages installed, you can use the
following command to add them to your C3 project:

```bash
cd <c3-simulator-root>
make riscv-configure_simics
```

One the project is updated, you can build the Simics buildroot
environment with:

```bash
cd <c3-simulator-root>
make riscv-buildroot-config
make riscv-buildroot-build
```

Then make sure to rebuild all modules and run with:

```bash
# Rebuild Simics modules
make -B
# Run Simics with the RISC-V C3 module and buildroot
make riscv-run
```

If this boots successfully, you can either interact with Simics
as regular (e.g., if running in terminal, press `ctrl+C` to pause Simics and return to the Simics shell).


Alternatively, you can use the following to access via SSH. This relies
on Simics forwarding connections to `localhost:4022` into the Simics
environment and may, depending on your system setup, not work reliably.
The SSH makefile targets are:

```bash
# Run test workloads via scp and ssh:
## Test Hello World (no C3)
make riscv-hello
## Test software-generated cryptographic addresses (CAs)
make riscv-hello_riscv_c3

# SSH into simulated environment
make riscv-ssh
```
