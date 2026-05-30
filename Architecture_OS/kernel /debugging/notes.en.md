# Lecture Notes — System and Kernel Debugging

> **Module**: OS Architecture — ARSE · ENSIIE  
> **Lecturer**: Aurélien Cedeyn · 2021–2022  
> **Topics**: userspace debugging, kernel debugging, `strace`, `ltrace`, `gdb`, `/proc`, `dmesg`, `perf`, `crash`, kdump, BPF.

---

> **Tool selection guide**
>
> | Problem | Tool |
> |---|---|
> | List active processes | `ps aux`, `top` |
> | Trace system calls | `strace` |
> | Trace library calls | `ltrace` |
> | Crashing program | `gdb`, core file |
> | Blocked process | `gdb -p PID`, `strace -p PID` |
> | Kernel messages | `dmesg -T` |
> | Kernel function tracing | `perf`, `ftrace`, BPF/BCC |
> | Kernel panic | `kdump`, `crash` |
> | Navigate large C codebase | `cscope` |

---

## Table of Contents

1. [Debugging overview](#1-debugging-overview)
2. [The three system layers](#2-the-three-system-layers)
3. [Assembly basics for debugging](#3-assembly-basics-for-debugging)
4. [From source to binary](#4-from-source-to-binary)
5. [Process memory layout](#5-process-memory-layout)
6. [The call stack](#6-the-call-stack)
7. [Userspace tools: /proc, ps, top](#7-userspace-tools-proc-ps-top)
8. [strace and ltrace](#8-strace-and-ltrace)
9. [gdb — userspace debugger](#9-gdb--userspace-debugger)
10. [Core files and post-mortem analysis](#10-core-files-and-post-mortem-analysis)
11. [Kernel internals: task_struct, mm_struct](#11-kernel-internals-task_struct-mm_struct)
12. [System calls in the kernel](#12-system-calls-in-the-kernel)
13. [Kernel logging: dmesg](#13-kernel-logging-dmesg)
14. [debugfs, dynamic_debug, ftrace](#14-debugfs-dynamic_debug-ftrace)
15. [perf — tracing and statistics](#15-perf--tracing-and-statistics)
16. [crash — kernel debugger](#16-crash--kernel-debugger)
17. [kdump and kexec](#17-kdump-and-kexec)
18. [BPF/BCC and SystemTap](#18-bpfbcc-and-systemtap)
19. [Practical debugging methodology](#19-practical-debugging-methodology)
20. [Essential command reference](#20-essential-command-reference)

---

## 1. Debugging Overview

Debugging means finding, understanding, and fixing errors or abnormal behavior in a system.

The process:
1. Observe the symptom.
2. Form a hypothesis.
3. Choose the right tool.
4. Collect traces.
5. Relate traces to system internals.
6. Confirm or reject the hypothesis.

Types of analysis: reading logs, tracing processes, examining the call stack, inspecting memory, observing kernel behavior, post-mortem crash analysis.

---

## 2. The Three System Layers

```
┌────────────────────────────────┐
│ User Space                     │
│  shell, GUI, daemons, libc     │
├────────────────────────────────┤
│ Kernel Space                   │
│  system calls, drivers, modules│
├────────────────────────────────┤
│ Hardware                       │
│  CPU, memory, devices          │
└────────────────────────────────┘
```

- User space → kernel: via **system calls** (`syscall` instruction).
- Kernel → hardware: via **drivers**.

---

## 3. Assembly Basics for Debugging

Key x86-64 registers:

| Register | Role |
|---|---|
| `%rax` | return value; syscall number |
| `%rdi` | 1st argument |
| `%rsi` | 2nd argument |
| `%rdx` | 3rd argument |
| `%rbp` | base pointer (current stack frame) |
| `%rsp` | stack pointer (top of stack) |
| `%rip` | instruction pointer (next instruction) |

The `syscall` instruction triggers a controlled software interrupt, switching from user space to kernel space.

---

## 4. From Source to Binary

```c
void main(void) { exit(2); }
```

Equivalent assembly:
```asm
mov $60, %rax   # syscall number: exit
mov $2, %rdi    # exit code
syscall
```

Pipeline:
```
source.c → preprocessor → compiler → assembler → linker → executable
```

Useful inspection commands:
```bash
objdump -dsh prog.o   # disassemble and show sections
xxd prog.o            # raw byte dump
```

---

## 5. Process Memory Layout

```
┌──────────────────┐  High address
│   stack          │  ↓ grows down
├──────────────────┤
│   (free space)   │
├──────────────────┤
│   heap           │  ↑ grows up (malloc)
├──────────────────┤
│   bss            │  uninitialized globals
├──────────────────┤
│   data           │  initialized globals
├──────────────────┤
│   text           │  executable code
└──────────────────┘  Low address
```

---

## 6. The Call Stack

The call stack stores: function return addresses, local variables, saved registers, function arguments.

Key instructions:
- `call func` → pushes return address, jumps to `func`.
- `ret` → pops return address into `%rip`.
- `push %rbp` / `pop %rbp` → saves/restores the base pointer.

A stack trace (backtrace) shows the sequence of active function calls — crucial for locating a crash.

---

## 7. Userspace Tools: /proc, ps, top

`/proc` is a virtual filesystem exposing kernel and process information.

```bash
ls /proc/self/           # current process info
cat /proc/<PID>/status   # process status
cat /proc/<PID>/maps     # memory mappings
```

```bash
ps aux f         # all processes, forest view
top              # real-time process monitor
```

Key `ps` fields: PID, %CPU, %MEM, RSS (resident memory), STAT (state), COMMAND.

---

## 8. strace and ltrace

### strace — trace system calls

```bash
strace ./program                     # trace all syscalls
strace -e open,read,write ./program  # filter specific syscalls
strace -c ./program                  # summary statistics
strace -p <PID>                      # attach to running process
strace -f ./program                  # follow child processes
strace -tt ./program                 # include timestamps
```

### ltrace — trace library calls

```bash
ltrace -e malloc,free ./program      # trace specific library functions
ltrace -c ./program                  # summary
```

### Difference

| Tool | Observes |
|---|---|
| `strace` | Kernel system calls |
| `ltrace` | Dynamic library function calls |

---

## 9. gdb — Userspace Debugger

```bash
gdb ./program            # debug a binary
gdb -p <PID>             # attach to running process
gdb ./program core       # post-mortem with core file
```

Key commands:

| Command | Action |
|---|---|
| `break func` | Set breakpoint at function |
| `run [args]` | Start the program |
| `where` / `bt` | Show call stack |
| `continue` | Resume execution |
| `info registers` | Show register values |
| `disassemble func` | Show assembly |
| `print var` | Print variable value |
| `set var=value` | Modify a variable |
| `thread apply all bt` | Stack trace for all threads |
| `up` / `down` | Navigate stack frames |

---

## 10. Core Files and Post-Mortem Analysis

A **core file** is a snapshot of a process's memory at crash time.

```bash
ulimit -c unlimited          # enable core dumps
cat /proc/sys/kernel/core_pattern   # check naming pattern
gcore <PID>                  # manually generate core
gdb ./program core           # analyze it
```

In gdb after loading a core:
```gdb
(gdb) where
(gdb) info registers
(gdb) frame 0
(gdb) print variable
```

---

## 11. Kernel Internals: task_struct, mm_struct

### `task_struct`

The kernel structure representing a process or task. Contains:
- Process state, stack pointer, CPU assignment.
- Pointer to `mm_struct` (memory).
- Scheduling info, file descriptors, signals.

### `mm_struct`

Describes the process's memory space:

```
task_struct
  └── mm_struct
        ├── text (start_code / end_code)
        ├── data (start_data / end_data)
        ├── heap (start_brk / brk)
        ├── mmap regions (vm_area_struct list)
        └── stack (start_stack)
```

---

## 12. System Calls in the Kernel

Syscall numbers are architecture-specific:

```
arch/x86/entry/syscalls/syscall_64.tbl
```

Example:
```
0   common  read    sys_read
1   common  write   sys_write
60  common  exit    sys_exit
```

Kernel implementation uses `SYSCALL_DEFINEn` macros:

```c
SYSCALL_DEFINE1(exit, int, error_code)
{
    do_exit((error_code & 0xff) << 8);
}
```

### Process lifecycle: `clone` → `execve` → `exit`

```
shell
  ├── clone()   → creates child task_struct
  │     └── execve()  → replaces memory image with new binary
  │           └── exit()    → cleans up: mm, files, fs, signals
```

---

## 13. Kernel Logging: dmesg

```bash
dmesg               # print kernel ring buffer
dmesg -T            # human-readable timestamps
dmesg -w            # follow in real-time
dmesg | tail -50    # last 50 messages
```

Useful for: driver problems, hardware errors, kernel panics, module failures.

---

## 14. debugfs, dynamic_debug, ftrace

### debugfs

```bash
ls /sys/kernel/debug/
```

Exposes internal kernel debug interfaces.

### dynamic_debug

```bash
awk '$3 != "=_"' /sys/kernel/debug/dynamic_debug/control
```

Enables/disables specific kernel debug messages at runtime without recompiling.

### ftrace

Files in `/sys/kernel/debug/tracing/`:

| File | Role |
|---|---|
| `set_ftrace_pid` | Select process to trace |
| `current_tracer` | Set tracer type |
| `tracing_on` | Enable/disable tracing |
| `trace` | Read trace output |

---

## 15. perf — Tracing and Statistics

```bash
perf list                           # list available events
perf stat -a sleep 1                # system-wide CPU stats
perf record -e ext4:ext4_free_inode -a  # record event
perf report                         # visualize recording
perf probe -L vfs_open              # list source lines in function
perf probe vfs_open:8 path          # add probe at line 8
perf record -e probe:vfs_open_1 -aR sleep 1   # capture probe events
perf script                         # print captured events
```

---

## 16. crash — Kernel Debugger

`crash` is a GDB-like tool specialized for Linux kernel analysis (live or post-mortem).

```bash
sudo crash                      # live kernel analysis
crash vmlinux vmcore            # post-mortem with dump
```

Requires kernel debug symbols (`linux-image-XXX-dbgsym` on Debian/Ubuntu).

Key crash commands:
```
ps              list kernel tasks
set <PID>       select a task
bt              backtrace
bt -f           full backtrace
dis func        disassemble function
print sym       print symbol value
struct type     dump structure
kmem            memory info
rd              read memory
```

---

## 17. kdump and kexec

When the kernel panics, regular debugging tools are unavailable. **kdump** solves this:

1. At boot, reserve memory for a **capture kernel** (`crashkernel=auto` in boot args).
2. On panic, `kexec` boots the capture kernel instantly (no hardware reboot).
3. The capture kernel dumps the crashed kernel's memory to a file.
4. Analyze with `crash vmlinux vmcore`.

Panic behavior can be configured via `/proc/sys/kernel/panic*`.

---

## 18. BPF/BCC and SystemTap

### BPF/BCC

```bash
/usr/local/share/bcc/tools/gethostlatency
```

BPF programs run safely in the kernel and can trace: system calls, network, file access, scheduling — with minimal overhead. Ideal for production systems.

### SystemTap

Insert tracing probes at kernel points:

```systemtap
global reads
probe vfs.read {
    reads[execname()] <<< count
}
```

Can inspect variables, trace functions, and temporarily modify observed behavior.

---

## 19. Practical Debugging Methodology

### Slow userspace program
1. `top` / `htop` → check CPU, memory, state.
2. `strace -c` → identify dominant syscalls.
3. `ltrace -c` → identify expensive library calls.
4. `perf stat` → CPU-level bottleneck.
5. `gdb -p PID` → if process is blocked.

### Crashing program
```bash
ulimit -c unlimited
./program
gdb ./program core
# (gdb) where
# (gdb) info registers
# (gdb) frame 0
```

### Kernel problem
```bash
dmesg -T | tail -100          # read logs
awk '$3 != "=_"' /sys/kernel/debug/dynamic_debug/control  # enable debug msgs
perf probe -L vfs_open        # inspect kernel function
sudo perf record -a           # record system events
```

### Kernel panic
```bash
crash vmlinux vmcore
# (crash) ps; bt; log; kmem
```

---

## 20. Essential Command Reference

```bash
# Process inspection
ps aux f
top
cat /proc/<PID>/status
cat /proc/<PID>/maps

# Userspace tracing
strace ./prog
strace -e open,read,write ./prog
ltrace -e malloc,free ./prog

# GDB
gdb ./prog
gdb -p <PID>
gdb ./prog core

# Kernel logs
dmesg -T
dmesg -w

# perf
perf list
sudo perf stat -a sleep 1
sudo perf record -a
sudo perf report
sudo perf probe -L vfs_open

# crash
sudo crash
crash vmlinux vmcore
```
