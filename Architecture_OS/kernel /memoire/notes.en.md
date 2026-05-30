# Lecture Notes — Memory Allocation

> **Module**: OS Architecture — ARSE · ENSIIE  
> **Topics**: physical memory, virtual memory, malloc/free, paging, swap, TLB, page replacement, NUMA, HPC memory allocation, GPU Unified Memory.

---

> **Key concepts**
> - Page (logical) → Frame (physical) via page table; TLB = translation cache.
> - Page fault = page not in RAM → loaded from swap.
> - Replacement algorithms: FIF (optimal), LRU (practical), Clock (LRU approximation), FIFO (simple).
> - NUMA: local memory access is fast; first-touch policy places a page on the node of the first accessing thread.
> - CUDA Unified Memory: single CPU/GPU pointer + automatic migration (Pascal and beyond).
> - Thrashing = too much swapping → use working set model to prevent it.

---

## Table of Contents

1. [Physical and Logical Memory](#1-physical-and-logical-memory)
2. [Process Memory Layout](#2-process-memory-layout)
3. [Variable Types: Static, Automatic, Dynamic](#3-variable-types-static-automatic-dynamic)
4. [malloc and free](#4-malloc-and-free)
5. [Fragmentation](#5-fragmentation)
6. [Virtual Memory](#6-virtual-memory)
7. [Page Table and Address Translation](#7-page-table-and-address-translation)
8. [TLB](#8-tlb)
9. [Swap and Demand Paging](#9-swap-and-demand-paging)
10. [Page Fault](#10-page-fault)
11. [Page Replacement Algorithms](#11-page-replacement-algorithms)
12. [Thrashing and Working Set](#12-thrashing-and-working-set)
13. [Hierarchical Page Tables](#13-hierarchical-page-tables)
14. [mmap](#14-mmap)
15. [NUMA and First-Touch Policy](#15-numa-and-first-touch-policy)
16. [Memory Allocators in HPC](#16-memory-allocators-in-hpc)
17. [GPU Unified Memory](#17-gpu-unified-memory)
18. [Summary Cheatsheet](#18-summary-cheatsheet)

---

## 1. Physical and Logical Memory

**Physical memory (RAM)**: the actual hardware memory — an array of addressable cells.

- Sizes range from a few GB (phones) to hundreds of GB (HPC nodes).
- In NUMA systems, each CPU socket has its own local memory bank; remote access is slower.

**Logical (virtual) memory**: the address space seen by a program — isolated, continuous, and apparently private.

---

## 2. Process Memory Layout

```
┌──────────────────┐  High address
│   stack          │  local vars, return addresses  ↓ grows down
├──────────────────┤
│   (free space)   │
├──────────────────┤
│   heap           │  dynamic allocations (malloc)  ↑ grows up
├──────────────────┤
│   bss            │  uninitialized global variables
├──────────────────┤
│   data           │  initialized global variables
├──────────────────┤
│   text           │  executable code (read-only)
└──────────────────┘  Low address
```

---

## 3. Variable Types: Static, Automatic, Dynamic

| Type | Lifetime | Storage |
|---|---|---|
| **Static** | Entire program | Data / BSS segment |
| **Automatic** | Function scope | Stack |
| **Dynamic** | Programmer-controlled | Heap (`malloc`/`free`) |

```c
int global = 5;           // static (data segment)
void f() {
    int x = 3;            // automatic (stack)
    int *p = malloc(100); // dynamic (heap)
    free(p);              // must free manually
}
```

Forgetting `free` causes a **memory leak**.

---

## 4. malloc and free

```c
#include <stdlib.h>
void *malloc(size_t size);
void  free(void *ptr);
```

`malloc` and `free` operate in **user space**. The allocator calls the kernel only when it needs more memory:

| System call | Purpose |
|---|---|
| `brk` / `sbrk` | Extend or shrink the heap segment |
| `mmap` | Allocate an independent region (large allocations, ≥128 KB) |

The allocator **batches** kernel requests: it gets large blocks from the kernel and carves them up internally to satisfy multiple `malloc` calls.

### Why malloc can be slow in multithreaded programs

- Lock contention on internal allocator structures.
- False sharing between threads.
- Poor NUMA locality (all memory on one node).
- Page fault cost (new pages must be zeroed by the kernel for security).

---

## 5. Fragmentation

### External fragmentation

Free memory exists but is split into non-contiguous pieces — a large contiguous allocation fails even though total free space is sufficient.

### Internal fragmentation

Allocated block is larger than requested (alignment padding, minimum block size).

### Placement strategies

| Strategy | Rule | Weakness |
|---|---|---|
| First Fit | First block large enough | Fragments the beginning |
| Best Fit | Smallest block large enough | Leaves tiny unusable fragments |
| Worst Fit | Largest block | Wastes large regions |

---

## 6. Virtual Memory

Virtual memory provides each process with its own isolated address space and allows programs larger than physical RAM to run.

### Benefits

- Process isolation (different virtual addresses map to different physical frames).
- Demand paging (load only needed pages).
- Memory protection.
- Controlled sharing of physical pages.

### Pages and frames

| Term | Meaning |
|---|---|
| **Page** | Fixed-size block of logical memory |
| **Frame** | Fixed-size block of physical memory |

Typical page size: **4 KB** (2¹² bytes). Huge pages: 2 MB.

A logical address is split as:

```
64-bit address: [ page number (52 bits) ][ page offset (12 bits) ]
```

---

## 7. Page Table and Address Translation

Each process has its own **page table** (part of its context).

```
Page 0 → Frame 10
Page 1 → Frame 3
Page 2 → (on disk)
Page 3 → Frame 25
```

Address translation:

```
logical address = (page p, offset d)
page table[p] = frame f
physical address = frame f + offset d
```

The OS also maintains a **frame table** tracking which frames are free and which process/page occupies each frame.

---

## 8. TLB

**TLB (Translation Lookaside Buffer)**: a small, fast hardware cache for recent page-to-frame translations.

- **TLB hit**: translation found → fast access.
- **TLB miss**: translation not found → must read page table in RAM → slower.

On a context switch, the TLB may be flushed (or entries tagged with an Address Space Identifier on some architectures).

---

## 9. Swap and Demand Paging

When total page demand exceeds physical RAM, the OS uses **swap** (a disk partition or file) to store less-used pages.

**Demand paging**: a page is loaded only when first accessed (lazy loading).

If both RAM + swap are exhausted, the Linux **OOM Killer** terminates a memory-hungry process.

---

## 10. Page Fault

A **page fault** occurs when a process accesses a page not currently in RAM.

### Types of page fault

1. **Invalid reference**: illegal address → segmentation fault.
2. **Page in memory**: no fault; continue normally.
3. **Page on disk**: OS must load it.

### Page fault handling steps

1. Hardware detects missing page → traps to OS.
2. OS locates page on disk.
3. OS finds a free frame (or evicts a victim).
4. OS loads the page into the frame.
5. OS updates the page table.
6. OS restarts the faulting instruction.

### Page table bits

- **Valid bit (V)**: 1 = page in RAM, 0 = page on disk (or invalid).
- **Dirty bit (D)**: 1 = page has been modified → must write back before eviction.

---

## 11. Page Replacement Algorithms

When no free frame is available, a **victim page** must be evicted.

| Algorithm | Rule | Strength | Weakness |
|---|---|---|---|
| **FIF** (optimal) | Evict page used furthest in the future | Theoretically optimal | Requires knowing the future |
| **LRU** | Evict least recently used page | Good in practice | Expensive to implement exactly |
| **Clock** | Approximate LRU with a reference bit | Low overhead | Slight approximation |
| **FIFO** | Evict oldest loaded page | Simple | Ignores usage frequency |
| **Random** | Evict a random page | Very simple | Can evict critical pages |

**Theoretical ranking**: FIF > LRU > Clock > FIFO > Random

### Clock algorithm

Each page has a **reference bit** (set to 1 when accessed).

```
while true:
    if ref_bit(current) == 0:
        evict this page; stop
    else:
        ref_bit(current) = 0
        advance to next page
```

---

## 12. Thrashing and Working Set

### Thrashing

The system spends more time loading/unloading pages than executing programs.

Symptoms: very high page fault rate, high disk I/O, low effective CPU utilization.

Cause: too many processes compete for too few frames.

### Working Set

The **working set** of a process = set of pages accessed in the recent time window.

If `sum of all working sets > physical RAM` → suspend a process to free frames.

```
Few frames  → many page faults
Enough frames → few page faults
Too many frames → wasted memory
```

---

## 13. Hierarchical Page Tables

A flat 64-bit page table would be enormous (2⁵² entries).

Solution: multi-level page tables — only create sub-tables that are actually needed.

```
Level 1 table
   └── Level 2 table
         └── Level 3 table
               └── Frame number
```

Most 64-bit systems use 4 levels (x86-64) or 3 levels.

---

## 14. mmap

```c
void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
```

Maps a file (or anonymous memory) directly into the process's address space.

Benefits:
- Access file data as if it were memory.
- Demand paging: only needed pages are loaded.
- Avoids explicit `read`/`write` system calls.
- Can overlap computation and I/O.

---

## 15. NUMA and First-Touch Policy

In **NUMA** (Non-Uniform Memory Access) systems, CPUs have local memory banks. Accessing local memory is faster than accessing remote memory.

### First-Touch Policy (Linux default)

A physical page is allocated on the NUMA node of the **first thread** that accesses it.

```c
// BAD for NUMA: thread 0 initializes all data → all pages on node 0
// GOOD: parallelize initialization with the same threads that will use it
#pragma omp parallel for
for (int i = 0; i < N; i++) tab[i] = 0;
```

### numactl

```bash
numactl --hardware                       # show NUMA topology
numactl --physcpubind=3 --membind=0 ./prog  # bind CPU and memory
numactl --interleave=all ./prog          # interleave memory across nodes
```

---

## 16. Memory Allocators in HPC

### Problems with standard allocators in HPC

- Lock contention in multithreaded malloc.
- No NUMA awareness (all memory ends up on one node).
- Thread migration breaks locality.
- **False sharing**: two threads modify different variables on the same cache line.

### Solutions

**Per-thread memory pools**: each thread manages its own allocation arena.

```
OS / Memory Source
    ↓ macro-blocks (>2 MB)
NUMA pool (per NUMA node)
    ↓ smaller blocks
Thread pool (per thread)
    ↓ malloc() calls
Application
```

### Page zeroing cost

The kernel zeros new pages before giving them to a process (security). This can be a significant overhead in HPC workloads.

Optimization: reuse dirty pages within the same process (pages already belong to it → no security risk).

---

## 17. GPU Unified Memory

### Traditional CUDA (explicit copies)

```c
cudaMalloc(&d_data, N);
cudaMemcpy(d_data, h_data, N, cudaMemcpyHostToDevice);
gpu_kernel<<<...>>>(d_data, N);
cudaMemcpy(h_data, d_data, N, cudaMemcpyDeviceToHost);
```

Drawbacks: verbose code, manual synchronization, complex for irregular data structures.

### CUDA Unified Memory

```c
cudaMallocManaged(&data, N);
cpu_func(data, N);
gpu_kernel<<<...>>>(data, N);
cudaDeviceSynchronize();
cpu_func(data, N);
cudaFree(data);
```

A single pointer, accessible from both CPU and GPU. The driver handles page migrations automatically.

### GPU Architecture Evolution

| Architecture | Key feature |
|---|---|
| **Kepler** (CUDA 6) | Unified Memory introduced; bulk migration at kernel launch |
| **Pascal** | GPU page faults; on-demand migration; oversubscription |
| **Volta** | Access counters; smarter migration decisions |
| **Volta + NVLink2** | Cache coherency; CPU direct access to GPU memory |

### User-provided hints

```c
// Prefetch pages to GPU before kernel launch
cudaMemPrefetchAsync(data, N, gpuId, stream);

// Declare data as mostly read → driver may replicate it
cudaMemAdvise(data, N, cudaMemAdviseSetReadMostly, gpuId);

// Declare preferred location
cudaMemAdvise(data, N, cudaMemAdviseSetPreferredLocation, cudaCpuDeviceId);
```

---

## 18. Summary Cheatsheet

```
Physical memory = actual RAM
Logical memory  = program's virtual address space
Page   = fixed-size logical block (typically 4 KB)
Frame  = fixed-size physical block
Page table: page → frame (one per process)
TLB    = hardware cache of recent translations
Swap   = disk area for evicted pages
Page fault = page not in RAM → load it

Valid bit: 1 = in RAM, 0 = on disk
Dirty bit: 1 = modified → must write back

FIF    = optimal (future-aware, theoretical)
LRU    = evict least recently used
Clock  = approximate LRU with reference bit
FIFO   = evict oldest

Thrashing = CPU busy swapping, not computing
Working set = recently used pages

mmap   = map file/memory into address space
NUMA   = non-uniform memory access latency
First-touch = page placed near the first accessor

CUDA Unified Memory = single pointer CPU+GPU, auto migration
Pascal = GPU page faults + on-demand migration
```
