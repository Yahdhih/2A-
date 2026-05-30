# Lecture Notes — Process Scheduling

> **Module**: OS Architecture — ARSE · ENSIIE  
> **Topics**: processes, UNIX primitives, `fork`, `wait`, `execve`, IPC, signals, semaphores, threads, scheduling algorithms, multiprocessor scheduling, Linux schedulers `O(n)`, `O(1)`, and **CFS**.

---

> **Key takeaways**
> - `fork` duplicates a process (copy-on-write); returns child PID in parent, 0 in child.
> - `execve` replaces the memory image without changing the PID.
> - CFS picks the task with the smallest `vruntime`, stored in a red-black tree.
> - `O(1)` scheduler: active/expired queues + bitmap for constant-time selection.
> - Starvation → fix with priority aging.
> - Process migration between CPUs is expensive (cache invalidated).

---

## Table of Contents

1. [Processes](#1-processes)
2. [UNIX Primitives](#2-unix-primitives)
3. [Process States](#3-process-states)
4. [IPC: Signals and Semaphores](#4-ipc-signals-and-semaphores)
5. [Threads](#5-threads)
6. [Scheduling Algorithms](#6-scheduling-algorithms)
7. [Multiprocessor Scheduling](#7-multiprocessor-scheduling)
8. [Linux Schedulers](#8-linux-schedulers)
9. [CFS: Completely Fair Scheduler](#9-cfs-completely-fair-scheduler)
10. [Quick Reference](#10-quick-reference)

---

## 1. Processes

A **process** is a running instance of a program. It contains:
- Its own address space (code, data, heap, stack)
- CPU registers state
- Open file descriptors
- Signals, priority, scheduling state
- Page table

A process can run in two modes:
- **User mode**: executes ordinary instructions, restricted memory access.
- **Kernel mode**: executing a system call, can run privileged instructions.

---

## 2. UNIX Primitives

### `fork` — create a child process

```c
pid_t fork(void);
```

| Return value | Context |
|---|---|
| Child's PID (> 0) | In the parent |
| 0 | In the child |
| -1 | Error |

Under Linux, `fork` uses **copy-on-write**: memory pages are shared until one process writes to them, then a real copy is made.

### `execve` — replace the current program

```c
int execve(const char *path, char *const argv[], char *const envp[]);
```

Replaces the process's code, data, heap and stack with a new program. The PID stays the same. On success, `execve` **never returns**.

### Classic pattern: `fork` + `execve`

```c
pid_t pid = fork();
if (pid == 0) {
    char *args[] = {"/bin/ls", NULL};
    execv(args[0], args);
}
// parent continues or calls wait()
```

### `wait` / `waitpid` — reap a child

```c
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
```

A **zombie process** is a terminated child whose parent has not yet called `wait`. The kernel keeps its entry in the process table until the parent retrieves the exit status.

---

## 3. Process States

```
New → Ready → Running (user) → Running (kernel) → Sleeping
                   ↓
                Zombie
```

| State | Meaning |
|---|---|
| New | Just created |
| Ready | Has resources, waiting for CPU |
| Running (user) | Executing user-mode instructions |
| Running (kernel) | Executing a system call or handling an interrupt |
| Sleeping | Waiting for an event (I/O, signal, child) |
| Suspended | Stopped by `SIGSTOP` or `SIGTSTP` |
| Zombie | Terminated, not yet waited on |

---

## 4. IPC: Signals and Semaphores

### Signals

A signal is a **software interrupt** sent to a process.

Common signals:

| Signal | Number | Default action |
|---|---:|---|
| `SIGHUP` | 1 | Hangup |
| `SIGINT` | 2 | Interrupt (Ctrl+C) |
| `SIGKILL` | 9 | Kill immediately (cannot be caught) |
| `SIGSEGV` | 11 | Segmentation fault |
| `SIGTERM` | 15 | Graceful termination request |
| `SIGSTOP` | 19 | Pause (cannot be caught) |

```c
kill(pid, SIGTERM);    /* send signal to a process */
raise(SIGINT);         /* send signal to self */
```

Custom handlers via `sigaction`:

```c
struct sigaction act = { .sa_handler = my_handler };
sigaction(SIGINT, &act, NULL);
```

### Semaphores

A semaphore is an integer that can never go below zero.

```c
sem_wait(&sem);   /* decrement; block if 0 */
sem_post(&sem);   /* increment; wake a waiter */
```

- **Named semaphore**: identified by `/name`, opened with `sem_open()`.
- **Anonymous semaphore**: in shared memory, initialized with `sem_init()`.

---

## 5. Threads

A thread is a **lightweight process**. Threads within the same process share:
- Address space, code, global variables, heap, open files.

Each thread owns:
- Its own stack, registers, and context.

| | Process | Thread |
|---|---|---|
| Address space | Separate | Shared |
| Communication | Costly (IPC) | Direct (shared memory) |
| Context switch | Expensive | Cheaper |
| Creation | More expensive | Lighter |

---

## 6. Scheduling Algorithms

### Round Robin

Each process gets a fixed time quantum; preempted process goes to the end of the queue.

- Simple and fair.
- Ignores priority; quantum size matters.

### Priority Scheduling

The scheduler always picks the highest-priority ready process.

- Expressiveness: critical processes can be favored.
- **Starvation**: low-priority processes may never run.
- **Fix**: priority aging — slowly increase the priority of waiting processes.

### Multilevel Queues

Processes are split into classes (real-time, system, interactive, batch). Each class has its own queue and scheduling policy.

### Multilevel Feedback Queues (MLFQ)

Processes can move between queues based on behavior:
- Use full quantum → likely CPU-bound → demoted to lower-priority queue.
- Surrender CPU early → likely I/O-bound → stays in high-priority queue.

Weakness: processes can "game" the scheduler by sleeping just before the quantum ends.

---

## 7. Multiprocessor Scheduling

In multiprocessor systems, the scheduler must decide:
- Which process to run?
- On which CPU?
- When to migrate a process?

### Process migration cost

Migration breaks CPU cache locality — data must be reloaded on the new CPU. This is why schedulers prefer **soft affinity** (prefer to keep a process on the same CPU).

### Queue architectures

| Approach | Advantage | Disadvantage |
|---|---|---|
| Single global queue | Fair, simple | Not scalable, high contention |
| Per-CPU queues | Scalable, cache-friendly | Load imbalance |
| Hybrid | Best of both | More complex |

**Load balancing**:
- **Push migration**: a task periodically moves processes from overloaded CPUs.
- **Pull migration**: an idle CPU steals a process from a busy CPU.

---

## 8. Linux Schedulers

### `O(n)` scheduler (Linux 2.4–2.6)

Scans all ready processes at every context switch to find the best one.

- Time complexity: O(n) — not scalable.
- Used a single global run queue → bad for SMP.

### `O(1)` scheduler (Linux 2.6–2.6.22)

Two run queue arrays per CPU: **active** and **expired**.

Each contains 140 priority levels (0–99 real-time, 100–139 normal).

Selection: find the first non-empty queue using a bitmap + hardware `find-first-bit-set` → **O(1)** per decision.

**Dynamic priority** formula:

```
dynamic_priority = MAX(100, MIN(static_priority - bonus + 5, 139))
```

Bonus is based on average sleep time:
- High sleep time (I/O-bound) → higher bonus → better priority.
- Low sleep time (CPU-bound) → lower bonus → worse priority.

**Time slice** formula:

```
priority < 120 → time_slice = (140 - priority) × 20 ms
priority ≥ 120 → time_slice = (140 - priority) × 5 ms
```

---

## 9. CFS: Completely Fair Scheduler

CFS has been the default Linux scheduler since version 2.6.23 (2007).

### Core idea

Give each runnable task a fair share of the CPU. With N ready tasks, each should get ~`100/N %` of CPU time.

### `vruntime`

Each task tracks a **virtual runtime** (`vruntime`) — the amount of CPU time it has received (weighted by priority).

```
vruntime += time_on_cpu × weight_factor(nice)
```

- Low-priority task: weight > 1 → `vruntime` grows faster → scheduled less.
- High-priority task: weight < 1 → `vruntime` grows slower → scheduled more.

### Red-black tree

CFS stores all runnable tasks in a **self-balancing red-black tree**, ordered by `vruntime`.

- Leftmost node = smallest `vruntime` = next task to run.
- Insert/delete: O(log n).
- CFS caches the leftmost node as `min_vruntime`.

### Scheduling loop

1. Pick leftmost node (smallest `vruntime`).
2. Run the task.
3. Update `vruntime`.
4. Reinsert into the tree.

A CPU-bound task's `vruntime` grows fast → moves right → scheduled less.  
An I/O-bound task's `vruntime` stays low → stays left → gets CPU quickly when it wakes.

### New tasks

A new task starts with `vruntime ≈ min_vruntime` so it gets a turn quickly without starving others.

---

## 10. Quick Reference

### Key formulas

```
# O(1) scheduler
dynamic_priority = MAX(100, MIN(static_priority - bonus + 5, 139))
time_slice = (140 - priority) × 20ms   if priority < 120
time_slice = (140 - priority) × 5ms    if priority ≥ 120

# CFS ideal fairness
CPU share per task = 100 / N %

# CFS vruntime update
vruntime += execution_time × nice_weight
```

### Scheduler comparison

| Scheduler | Key idea | Strength | Weakness |
|---|---|---|---|
| Round Robin | Fixed quantum, take turns | Simple, fair | Ignores priority |
| Priority | Always pick highest priority | Expressive | Starvation |
| MLFQ | Dynamic promotion/demotion | Adapts to behavior | Can be gamed |
| O(n) Linux | Scan all tasks | Simple | Not scalable |
| O(1) Linux | Bitmap + active/expired queues | Constant-time pick | Complex heuristics |
| CFS | Smallest vruntime first | Elegant fairness | More complex data structure |
