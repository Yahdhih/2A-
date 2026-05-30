# Lecture Notes — System and Kernel Optimization

> **Module**: OS Architecture — ARSE · ENSIIE  
> **Lecturer**: Aurélien Cedeyn · 2021–2022  
> **Topics**: optimization levels, UMA/SMP/NUMA architectures, kernel configuration, `ulimit`, `sysctl`, kernel modules, tickless mode, CPU isolation, NUMA placement, cgroups, Flamegraph, `perf`.

---

> **Golden rule**: measure → understand → change → measure again. Never optimize without data.
>
> **Key commands**
> ```bash
> ulimit -a                    # process resource limits
> sysctl -a                    # kernel parameters
> numactl --hardware           # NUMA topology
> perf record / perf report    # profiling
> cat /proc/cmdline            # kernel boot options
> lsmod / modinfo <module>     # kernel modules
> ```

---

## Table of Contents

1. [What is optimization?](#1-what-is-optimization)
2. [Hardware, Software, and System Optimization](#2-hardware-software-and-system-optimization)
3. [Hardware Evolution: GPU, RDMA, CPU](#3-hardware-evolution-gpu-rdma-cpu)
4. [UMA, SMP, and NUMA](#4-uma-smp-and-numa)
5. [Kernel Configuration](#5-kernel-configuration)
6. [ulimit](#6-ulimit)
7. [/proc/sys and sysctl](#7-procsys-and-sysctl)
8. [Kernel Modules](#8-kernel-modules)
9. [Tickless Mode: NO_HZ_FULL](#9-tickless-mode-no_hz_full)
10. [CPU Isolation: isolcpus](#10-cpu-isolation-isolcpus)
11. [NUMA Placement: numactl](#11-numa-placement-numactl)
12. [Resource Control: cgroups](#12-resource-control-cgroups)
13. [Performance Analysis Tools](#13-performance-analysis-tools)
14. [Flamegraph](#14-flamegraph)
15. [perf probe](#15-perf-probe)
16. [Optimization Methodology](#16-optimization-methodology)
17. [Command Reference](#17-command-reference)

---

## 1. What is Optimization?

Optimization means making something more efficient, more functional, or better suited to a specific goal.

In system optimization:
- **Not just making code faster** — it includes hardware usage, OS configuration, process placement, and measurement.
- A poorly measured optimization may move the bottleneck rather than eliminate it.

Key insight: the same code on a single machine can be correct and fast, but collapse when 1000 machines simultaneously hit the same shared resource (e.g., a network file server).

---

## 2. Hardware, Software, and System Optimization

| Level | What changes | Example |
|---|---|---|
| **Hardware** | Use better or different hardware | Delegate to GPU, use RDMA, pick MCDRAM |
| **Software** | Modify the application | Change algorithm, reduce I/O, vectorize |
| **System** | Configure or modify the OS | `sysctl`, `numactl`, `isolcpus`, cgroups |

In HPC, the system must minimize interference with compute processes:
- No unnecessary interrupts.
- No migration of compute threads.
- Memory close to the executing CPU.

---

## 3. Hardware Evolution: GPU, RDMA, CPU

### GPU

- Excellent for massively parallel workloads: matrix math, simulations, ML.
- High performance-per-watt ratio.

### RDMA (Remote Direct Memory Access)

- One machine directly reads/writes another's memory, bypassing the OS stack.
- Benefits: lower latency, fewer copies, less CPU overhead — critical in HPC clusters.

### CPU

- More cores, wider SIMD, deeper caches, NUMA topology.
- System optimization must evolve with CPU architecture.

---

## 4. UMA, SMP, and NUMA

### UMA / SMP — Uniform Memory Access

All CPUs share the same memory bus with equal latency.

Limitation: as the number of CPUs grows, the memory bus becomes a bottleneck → performance stops scaling.

### NUMA — Non-Uniform Memory Access

Each CPU socket (or group) has its own local memory. Cross-socket access is slower.

```
CPU 0 ──── Memory Node 0
CPU 1 ──── Memory Node 1
```

NUMA implications:
- Process placement (which CPU runs the code?)
- Memory placement (which node holds the data?)
- I/O placement (which node handles interrupts?)
- Thread migration breaks locality.

Two executions of the same program on a NUMA machine can have very different performance depending on placement.

---

## 5. Kernel Configuration

The kernel is compiled with a specific set of options stored in a config file.

```bash
cat /boot/config-$(uname -r)   # distribution config
zcat /proc/config.gz            # if CONFIG_IKCONFIG_PROC is set
cat /proc/cmdline               # boot-time parameters
```

Some important config options for HPC:
```
CONFIG_NO_HZ_FULL      # full tickless mode
CONFIG_PREEMPT         # kernel preemption
CONFIG_HZ=1000         # timer frequency
```

Boot parameters are documented in `Documentation/kernel-parameters.txt`.

---

## 6. ulimit

`ulimit` controls per-process resource limits set by the shell.

```bash
ulimit -a                  # list all current limits
cat /proc/<PID>/limits     # limits for a specific process
```

Common limits:

| Limit | Flag | Description |
|---|---|---|
| Open files | `-n` | Max number of open file descriptors |
| Stack size | `-s` | Stack segment size |
| Core file size | `-c` | Max size of core dump |
| CPU time | `-t` | Max CPU time in seconds |
| Max processes | `-u` | Max number of user processes |
| Virtual memory | `-v` | Max virtual memory |

### Persistent configuration

```bash
# /etc/security/limits.conf
@student hard nproc 20
@faculty soft nproc 20
@faculty hard nproc 50
```

Applied via PAM (Pluggable Authentication Modules).

---

## 7. /proc/sys and sysctl

`/proc/sys` exposes kernel tuning parameters as virtual files.

Key subdirectories:

| Path | Controls |
|---|---|
| `/proc/sys/vm` | Virtual memory behavior |
| `/proc/sys/kernel` | General kernel settings |
| `/proc/sys/net` | Network stack |
| `/proc/sys/fs` | Filesystem limits |

### Using sysctl

```bash
sysctl -a                           # list all parameters
sysctl vm.swappiness                # read one parameter
sudo sysctl vm.swappiness=10        # change temporarily
```

### Permanent changes

```bash
# /etc/sysctl.d/99-custom.conf
vm.swappiness = 10
fs.file-max = 1000000
```

```bash
sudo sysctl --system    # apply all sysctl.d files
```

**Warning**: always measure before and after changing kernel parameters. A wrong setting can degrade or destabilize the system.

---

## 8. Kernel Modules

A kernel module is loadable code that extends the kernel without recompiling it.

```bash
lsmod                       # list loaded modules
modinfo kvm                 # module metadata and parameters
sudo modprobe kvm           # load a module (handles dependencies)
insmod module.ko            # load directly (no dependency resolution)
```

### Module parameters

```bash
# /etc/modprobe.d/custom.conf
options kvm ignore_msrs=1
```

```bash
ls /sys/module/kvm/parameters/   # inspect loaded module parameters
```

---

## 9. Tickless Mode: NO_HZ_FULL

### The problem

Historically, the kernel sent a periodic timer interrupt (**tick**) to all CPUs. This wakes up even idle CPUs and disrupts compute processes.

### Solutions

| Mode | Behavior |
|---|---|
| `NO_HZ_IDLE` | Idle CPUs stop receiving ticks |
| `NO_HZ_FULL` | Even busy CPUs with a single runnable task stop receiving ticks |

### Why it matters for HPC

Ticks cause jitter: small unpredictable delays in compute processes. Eliminating ticks improves:
- Timing stability.
- Energy efficiency.
- MPI barrier synchronization (one slow process delays all others).

### Enabling

```
# Kernel config
CONFIG_NO_HZ_FULL=y

# Boot parameter
nohz_full=1-15     # CPUs 1 to 15 are tickless (CPU 0 must remain ticked)
```

---

## 10. CPU Isolation: isolcpus

### The problem

Linux's scheduler places system daemons, kernel threads, and user tasks on any available CPU. This creates noise on CPUs intended for HPC workloads.

### isolcpus

Removes specified CPUs from the general scheduler pool.

```
# Boot parameter
isolcpus=2,4-8,12
```

After isolation:
- The scheduler will not automatically place tasks on isolated CPUs.
- You must explicitly assign processes there using `numactl`, `taskset`, or your job scheduler (e.g., Slurm).

### Limits

- Requires explicit process placement.
- Can under-utilize the machine if not managed carefully.
- Some multi-threaded programs (e.g., OpenMP) handle CPU placement better internally.

---

## 11. NUMA Placement: numactl

```bash
numactl --hardware                            # show NUMA topology
numactl --show                                # show current policy
numactl --physcpubind=3 --membind=0 ./prog   # run on CPU 3, memory from node 0
numactl --interleave=all ./prog              # interleave memory across all nodes
numactl --localalloc ./prog                  # prefer local memory
```

### Underlying system calls

```c
sched_getaffinity(pid, sizeof(cpuset), &cpuset);   /* get CPU affinity */
sched_setaffinity(pid, sizeof(cpuset), &cpuset);   /* set CPU affinity */
```

### NUMA in practice

```bash
numactl --hardware
# available: 2 nodes (0-1)
# node 0 cpus: 0 1 2 3
# node 1 cpus: 4 5 6 7
# node distances: 0→0: 10, 0→1: 21
```

A process running on CPU 4 that allocates memory on node 0 pays the remote access penalty.

---

## 12. Resource Control: cgroups

**cgroups** (control groups) limit and isolate resource usage for groups of processes.

```bash
ls /sys/fs/cgroup/    # cgroup hierarchy
```

### What cgroups can limit

| Subsystem | Controls |
|---|---|
| `cpu` | CPU shares |
| `cpuset` | Which CPUs and memory nodes |
| `memory` | RAM and swap usage |
| `blkio` | Block I/O bandwidth |
| `devices` | Device access |
| `pids` | Number of processes |

### HPC use case

- Reserve CPUs for compute jobs.
- Prevent a runaway process from consuming all memory.
- Isolate job environments.

---

## 13. Performance Analysis Tools

Before optimizing, measure the actual bottleneck:

| Metric | Tool |
|---|---|
| CPU usage | `top`, `htop`, `perf stat` |
| Memory | `vmstat`, `numastat` |
| Disk I/O | `iostat`, `iotop` |
| Network | `nethogs`, `iperf` |
| System calls | `strace -c` |
| Kernel functions | `perf record` + `perf report` |
| NUMA behavior | `numastat`, `perf mem` |

---

## 14. Flamegraph

A **Flamegraph** is a visual representation of profiling traces.

- **Width** of a block = cumulative time spent in that function.
- **Height** = call depth (parents at bottom, children above).
- Immediately shows which functions dominate execution time.

Sources: `gdb`, `perf`, `SystemTap` traces.

Tools: [Brendan Gregg's Flamegraph scripts](http://www.brendangregg.com/flamegraphs.html).

---

## 15. perf probe

`perf probe` adds dynamic tracepoints to kernel or userspace functions.

```bash
# List source lines in a kernel function
perf probe -L vfs_open

# Add a probe at line 8, capturing 'path' argument
perf probe vfs_open:8 path

# Record events for 1 second system-wide
perf record -e probe:vfs_open_1 -aR sleep 1

# Print captured events
perf script
```

Use cases:
- Which process is calling a specific kernel function?
- How often is it called?
- What arguments are passed?

---

## 16. Optimization Methodology

### Step 1 — Understand context

- Application type? Hardware? NUMA? GPU? Shared filesystem?
- Batch or interactive workload?

### Step 2 — Measure

```bash
top; vmstat; iostat; perf stat; numactl --hardware; sysctl -a
```

### Step 3 — Identify the bottleneck

| Symptom | Likely cause |
|---|---|
| High CPU, low I/O | CPU-bound computation |
| High I/O wait | Disk or network bottleneck |
| Many page faults | Insufficient memory / bad locality |
| High kernel time | Too many syscalls, lock contention |
| Performance variance | Scheduler noise, NUMA misplacement |

### Step 4 — Apply one change at a time

| Problem | Fix |
|---|---|
| 1000 machines read the same file | Cache it, or read once and broadcast |
| HPC jitter | `isolcpus` + `nohz_full` |
| Bad memory placement | `numactl --membind` |
| Too many open files | `ulimit -n` |
| Slow kernel function | `perf probe` + Flamegraph |
| Wrong kernel parameter | `sysctl` |

### Step 5 — Document

Record: parameter changed, reason, before/after values, measured impact, rollback method.

---

## 17. Command Reference

```bash
# Kernel info
uname -a
cat /proc/cmdline
cat /boot/config-$(uname -r)

# Process limits
ulimit -a
cat /proc/<PID>/limits

# Kernel parameters
sysctl -a
sysctl vm.swappiness
sudo sysctl vm.swappiness=10
sudo sysctl --system

# Kernel modules
lsmod
modinfo kvm
sudo modprobe kvm
ls /sys/module/kvm/parameters

# NUMA
numactl --hardware
numactl --show
numactl --physcpubind=3 --membind=0 ./prog
numactl --interleave=all ./prog

# cgroups
ls /sys/fs/cgroup/

# perf
perf top
perf record ./prog
perf report
perf probe -L vfs_open
perf probe vfs_open:8 path
perf record -e probe:vfs_open_1 -aR sleep 1
perf script

# Boot parameters (add to grub/boot loader)
nohz_full=1-15
isolcpus=1-15
```
