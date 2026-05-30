# Analysis of `kernel/sched/completion.c`

> **Module**: OS Architecture — ARSE · ENSIIE  
> **File studied**: `kernel/sched/completion.c` (Linux kernel)  
> **Lecturer**: Aurélien Cedeyn  
> **Goal**: understand the Linux kernel `completion` primitive, its internals, and how it differs from semaphores.

---

## Table of Contents

1. [What is a completion?](#1-what-is-a-completion)
2. [Differences from semaphores](#2-differences-from-semaphores)
3. [`struct completion` structure](#3-struct-completion-structure)
4. [Source code analysis](#4-source-code-analysis)
5. [Main functions](#5-main-functions)
6. [Summary](#6-summary)

---

## 1. What is a completion?

A **completion** is a Linux kernel synchronization primitive that allows one thread to wait until a specific kernel activity has finished or a specific state has been reached.

It works similarly to `pthread_barrier()` in userspace: a thread waits for a `done` signal before continuing.

### Why use a completion rather than a mutex or semaphore?

A completion focuses on **one precise task** with a minimalist strategy. It is built on top of the kernel's `waitqueue` and `wakeup` infrastructure.

Threads that need to wait are put to sleep, then woken up by a simple signal stored in the `completion` structure.

---

## 2. Differences from semaphores

The source code explains this directly in its header comment:

```c
/*
 * Generic wait-for-completion handler;
 *
 * It differs from semaphores in that their default case is the opposite,
 * wait_for_completion default blocks whereas semaphore default non-block.
 * The interface also makes it easy to 'complete' multiple waiting threads,
 * something which isn't entirely natural for semaphores.
 *
 * But more importantly, the primitive documents the usage. Semaphores would
 * typically be used for exclusion which gives rise to priority inversion.
 * Waiting for completion is a typically sync point, but not an exclusion point.
 */
```

### Comparison table

| Criterion | Semaphore | Completion |
|---|---|---|
| Default behavior | Non-blocking | Blocking |
| Primary use | Protect resource access | Synchronize threads |
| Collective wake-up | Not natural | Yes (`complete_all`) |
| API readability | Generic | Explicit (`wait_for_completion`, `complete`) |
| Priority inversion risk | Yes | No |

**Semaphore**: the thread checks if the resource is free, uses it, or waits for it to become available.

**Completion**: the thread simply waits for the signal — it does not control any resource.

---

## 3. `struct completion` Structure

Defined in `include/linux/completion.h`:

```c
struct completion {
    unsigned int done;            /* completion counter */
    struct swait_queue_head wait; /* wait queue */
};
```

- `done`: counter tracking how many times `complete()` was called. `UINT_MAX` means "permanently done".
- `wait`: queue of sleeping threads waiting for the signal.

### Initialization

```c
/* Static */
DECLARE_COMPLETION(my_completion);

/* Dynamic */
struct completion c;
init_completion(&c);
```

---

## 4. Source Code Analysis

### License

```c
// SPDX-License-Identifier: GPL-2.0
```

GNU GPL v2 allows use, modification and redistribution provided sources are made available. `SPDX-License-Identifier` is a machine-readable annotation used by license analysis tools.

### Includes

```c
#include <linux/linkage.h>
#include <linux/sched/debug.h>
#include <linux/completion.h>
#include "sched.h"
```

---

## 5. Main Functions

### `complete_with_flags` — internal core

```c
static void complete_with_flags(struct completion *x, int wake_flags)
{
    unsigned long flags;

    raw_spin_lock_irqsave(&x->wait.lock, flags);

    if (x->done != UINT_MAX)
        x->done++;                         /* increment counter */
    swake_up_locked(&x->wait, wake_flags); /* wake one waiting thread */

    raw_spin_unlock_irqrestore(&x->wait.lock, flags);
}
```

- **`raw_spin_lock_irqsave`**: spinlock with interrupt disable — protects the critical section even in interrupt context.
- **`x->done != UINT_MAX`**: prevents overflow; `UINT_MAX` = "always done" state.
- **`swake_up_locked`**: wakes exactly one thread from the wait queue.

### `complete` — signal one thread

```c
void complete(struct completion *x)
{
    complete_with_flags(x, 0);
}
```

### `complete_on_current_cpu` — signal on current CPU

```c
void complete_on_current_cpu(struct completion *x)
{
    return complete_with_flags(x, WF_CURRENT_CPU);
}
```

NUMA optimization: preferentially wakes the thread on the same CPU for better cache locality.

### `complete_all` — wake all waiting threads

```c
void complete_all(struct completion *x)
{
    unsigned long flags;

    raw_spin_lock_irqsave(&x->wait.lock, flags);
    x->done = UINT_MAX;              /* mark as "permanently done" */
    swake_up_all_locked(&x->wait);   /* wake all waiters */
    raw_spin_unlock_irqrestore(&x->wait.lock, flags);
}
```

### `wait_for_completion` — blocking wait

```c
void wait_for_completion(struct completion *x)
{
    wait_for_common(x, MAX_SCHEDULE_TIMEOUT, TASK_UNINTERRUPTIBLE);
}
```

The thread enters `TASK_UNINTERRUPTIBLE` state: it cannot be woken by signals, only by `complete()`.

### `wait_for_completion_interruptible` — interruptible wait

```c
int wait_for_completion_interruptible(struct completion *x)
{
    long t = wait_for_common(x, MAX_SCHEDULE_TIMEOUT, TASK_INTERRUPTIBLE);
    return (t < 0) ? (int)t : 0;
}
```

Returns `-ERESTARTSYS` if a signal interrupts the wait.

### `wait_for_completion_timeout` — timed wait

```c
unsigned long wait_for_completion_timeout(struct completion *x,
                                          unsigned long timeout)
{
    return wait_for_common(x, timeout, TASK_UNINTERRUPTIBLE);
}
```

---

## 6. Summary

| Concept | Explanation |
|---|---|
| Role | Synchronize threads without protecting a resource |
| Mechanism | `waitqueue` + `done` counter |
| `complete()` | Wakes one sleeping thread |
| `complete_all()` | Wakes all sleeping threads, marks as permanent |
| `wait_for_completion()` | Blocks until signal, non-interruptible |
| Key difference vs semaphore | Blocking by default, collective wake-up, no priority inversion |

A completion is the ideal tool for **one-shot rendezvous points** between an initiator and one or more threads waiting for a single kernel event.
