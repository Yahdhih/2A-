# Lecture Notes — OS Architecture (Overview)

> **Module**: OS Architecture — ARSE · ENSIIE  
> **Topics**: memory management, allocators, scheduling, Linux kernel, filesystems, debugging, optimization.

---

## Module Overview

| Subtopic | File | Key concepts |
|---|---|---|
| Scheduling | [kernel /sched/notes.en.md](kernel%20/sched/notes.en.md) | `fork`, `execve`, CFS, O(1) scheduler, vruntime |
| Completion primitive | [kernel /sched/completion.en.md](kernel%20/sched/completion.en.md) | `completion` vs semaphore, `complete()`, `wait_for_completion()` |
| Memory | [kernel /memoire/notes.en.md](kernel%20/memoire/notes.en.md) | virtual memory, TLB, page replacement, NUMA, GPU Unified Memory |
| Debugging | [kernel /debugging/notes.en.md](kernel%20/debugging/notes.en.md) | `strace`, `gdb`, `perf`, `crash`, kdump, BPF |
| Optimization | [kernel /optimisation/notes.en.md](kernel%20/optimisation/notes.en.md) | `ulimit`, `sysctl`, `numactl`, `isolcpus`, cgroups, Flamegraph |
| Filesystems | [kernel /Local_filesystem/notes.en.md](kernel%20/Local_filesystem/notes.en.md) | VFS, FUSE, FAT, FFS, LFS, Ext4, ZFS, NOVA, LTFS |
| Memory TP | [tps/tp_mémoire/README.md](tps/tp_m%C3%A9moire/README.md) | `hp_allocator`, malloc wrapper, benchmarks |

---

## Key Themes

### 1. Memory: From Physical to Virtual

A process sees a continuous private address space. In reality, the OS, MMU, kernel, and allocators collaborate to provide this illusion:

```
Program (virtual addresses)
    ↓ malloc / mmap
User-space allocator (libc)
    ↓ brk / mmap syscall
Linux kernel (page tables, frames, swap)
    ↓ physical pages
RAM + NUMA topology
```

Key concepts:
- **Page** (4 KB logical block) ↔ **Frame** (4 KB physical block).
- **TLB**: hardware cache of recent page→frame translations.
- **Page fault**: page not in RAM → OS loads it from disk.
- **NUMA**: local memory is faster; first-touch determines placement.

### 2. Scheduling: Sharing the CPU Fairly

```
CPU-bound task:  vruntime grows fast → moves right in red-black tree
I/O-bound task:  vruntime stays low  → picked first when it wakes up
```

CFS (since Linux 2.6.23) keeps all runnable tasks in a red-black tree ordered by `vruntime`. The leftmost node (smallest `vruntime`) is always scheduled next.

### 3. Filesystems: Design Follows the Medium

| Storage | Key constraint | Filesystem response |
|---|---|---|
| HDD | Seek time | Locality (FFS cylinder groups) |
| HDD write-heavy | Write throughput | Sequential logging (LFS) |
| SSD/Flash | No random erase | Copy-on-write, GC |
| Persistent memory | Byte-addressable | Per-inode log, direct mapping (NOVA) |
| Tape | Sequential only | Linear index + streaming (LTFS) |

VFS provides a single POSIX interface (`open`, `read`, `write`, `stat`) over all of these.

### 4. Debugging: Always Measure First

```
Symptom user → ps / top → strace → ltrace → gdb
Symptom kernel → dmesg → perf → debugfs → crash
```

### 5. Optimization: Measure → Change → Measure

```
ulimit     → per-process resource limits
sysctl     → kernel tunable parameters (/proc/sys)
numactl    → NUMA-aware CPU and memory placement
isolcpus   → remove CPUs from scheduler pool (HPC)
nohz_full  → eliminate timer interrupts on compute CPUs
cgroups    → limit and isolate resources per process group
```

---

## Quick Reference: Essential Commands

```bash
# Memory & NUMA
numactl --hardware
numactl --physcpubind=0 --membind=0 ./prog
cat /proc/<PID>/maps

# Scheduling & processes
ps aux f
top
cat /proc/<PID>/status
nice -n -5 ./prog           # raise priority

# Kernel parameters
sysctl -a | grep vm
sudo sysctl vm.swappiness=10

# Debugging
strace -e open,read,write ./prog
gdb ./prog core
perf stat ./prog
dmesg -T | tail -50

# Modules
lsmod
modinfo <module>
sudo modprobe <module>

# Filesystem
df -h
mount | grep ext4
tune2fs -l /dev/sda1       # Ext4 superblock info

# Optimization (HPC)
cat /proc/cmdline           # check isolcpus / nohz_full
ls /sys/fs/cgroup/
perf record -a ./prog && perf report
```

---

## Key Formulas

```
# CFS fairness
CPU share per task = 100 / N  (%)

# vruntime update (simplified)
vruntime += execution_time × nice_weight

# O(1) scheduler: dynamic priority
dynamic_priority = MAX(100, MIN(static_priority - bonus + 5, 139))

# O(1) scheduler: time slice
time_slice = (140 - priority) × 20ms   if priority < 120
time_slice = (140 - priority) × 5ms    if priority ≥ 120

# Virtual address decomposition (x86-64, 4 KB pages)
address = [ page number (52 bits) ][ offset (12 bits) ]
```
