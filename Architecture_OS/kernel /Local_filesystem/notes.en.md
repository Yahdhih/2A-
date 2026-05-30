# Lecture Notes — Local Filesystems

> **Module**: OS Architecture — ARSE · ENSIIE  
> **Lecturer**: Jacques-Charles Lafoucrière  
> **Topics**: storage media, Linux VFS, FUSE, DOS/FAT, FFS, LFS, Ext2/3/4, ZFS/OpenZFS, NOVA, LTFS.

---

> **Quick summary**
>
> | Filesystem | Target media | Key idea |
> |---|---|---|
> | FAT | Floppy / small devices | Linked allocation table |
> | FFS | HDD | Locality (cylinder groups) |
> | LFS | HDD (write-heavy) | Write everything sequentially |
> | Ext4 | General Linux | FFS + journaling + extents |
> | ZFS | Advanced storage | CoW + checksums + pool |
> | NOVA | Persistent memory | Per-inode log, direct access |
> | LTFS | Magnetic tape | XML index + sequential data |
>
> **Fundamental principle**: filesystem design always depends on the target storage medium.

---

## Table of Contents

1. [What is a Filesystem?](#1-what-is-a-filesystem)
2. [Core Concepts](#2-core-concepts)
3. [Storage Media](#3-storage-media)
4. [Linux VFS](#4-linux-vfs)
5. [FUSE](#5-fuse)
6. [DOS / FAT](#6-dos--fat)
7. [FFS — Fast File System](#7-ffs--fast-file-system)
8. [LFS — Log-Structured File System](#8-lfs--log-structured-file-system)
9. [Ext2 / Ext3 / Ext4](#9-ext2--ext3--ext4)
10. [ZFS / OpenZFS](#10-zfs--openzfs)
11. [NOVA](#11-nova)
12. [LTFS](#12-ltfs)
13. [Comparative Summary](#13-comparative-summary)

---

## 1. What is a Filesystem?

A filesystem is:
1. An **organization of data** on a storage medium.
2. **Software** that controls how data is stored and retrieved.
3. An **abstraction** — applications work with files and directories, not raw blocks.

A raw storage device (`/dev/sdX`) is just a large byte sequence. The filesystem gives it structure.

### Key properties

| Property | Meaning |
|---|---|
| Speed | Fast access to files |
| Flexibility | Create, delete, move, extend files |
| Integrity | Read data == written data |
| Security | Access control |
| Reliability | Survive crashes, hardware faults |

> There is no single universal filesystem. The right choice depends on the storage medium, performance requirements, reliability needs, and access patterns.

---

## 2. Core Concepts

### Space management

The filesystem must track:
- Which blocks belong to which files.
- Which blocks are free.

Space is divided into fixed-size **blocks** (or clusters).

### Fragmentation

**External fragmentation**: free space exists, but is scattered — no contiguous region large enough for a large allocation.

**Internal fragmentation**: allocated block is larger than needed.

**Consequence for HDDs**: non-contiguous blocks require seek operations → slower I/O.

### Metadata

Every file has associated metadata:
- Size, creation/modification/access dates, owner, group, permissions.
- Pointers to data blocks.

Global metadata:
- Free-block bitmap, bad sector list, filesystem statistics.

### Crash consistency

Multiple metadata structures must be updated atomically (directory entry, inode, block bitmap). A crash at the wrong moment can leave the filesystem **inconsistent**.

Solutions: journaling, soft updates, copy-on-write.

### Advanced features

| Feature | Description |
|---|---|
| **Snapshots** | Point-in-time consistent copy of the filesystem |
| **Copy-on-write (CoW)** | Write new data at a new location; old data preserved for snapshots |
| **Clones** | Read-write snapshot |
| **Quotas** | Limit blocks/inodes per user/group/project |
| **Deduplication** | Store only one copy of identical blocks |
| **File locking** | Read/write/exclusive locks for concurrent access |

---

## 3. Storage Media

### HDD (Hard Disk Drive)

- Rotating magnetic platters, moving read/write head.
- Access time ~10 ms (seek + rotation).
- **Sequential access**: excellent. **Random access**: expensive.
- Filesystem impact: minimize head movement → favor **locality**.

### SSD (Solid State Drive)

- No moving parts; flash memory cells.
- Very fast and uniform access.
- Cannot overwrite in place → must erase before write.
- Erase is done per block; cells wear out after many writes.
- Needs garbage collection.

### Zoned Devices (SMR drives, ZNS SSDs)

- Address space divided into zones.
- Each zone has a write pointer; only sequential writes within a zone.
- Cannot overwrite in place; zone must be reset first.

### Magnetic Tape

- Linear sequential medium (>800 m long).
- Serpentine recording.
- Excellent for archiving large volumes (cheap, long lifetime).
- Very slow random access (minutes).

### Media Comparison

| Criterion | HDD | SSD | Tape |
|---|---|---|---|
| Capacity | Good | Good | Very large |
| Access time | ~10 ms | < 1 ms | Minutes (seek) |
| Sequential | Excellent | Excellent | Excellent |
| Random | Moderate | Excellent | Poor |
| Energy (idle) | Medium | Low | Very low |

---

## 4. Linux VFS

**VFS (Virtual File System)** is a kernel abstraction layer providing a uniform interface to all filesystems.

```c
write(fd, buf, len);   /* works whether fd is on ext4, FAT, or FUSE */
```

### VFS objects

| Object | Role |
|---|---|
| **Superblock** | Represents a mounted filesystem |
| **Inode** | Represents a file or directory (no name!) |
| **Dentry** | Represents a path component |
| **File** | Represents an open file in a process |

### Inode

Contains: type, access mode, inode number, timestamps, size, block count, link count, extended attribute size, **pointers to data blocks**.

Does **not** contain the filename. The name is stored in the directory. This enables **hard links**: multiple names pointing to the same inode.

### Dentry

Represents one component of a path:

```
/home/user/file.txt → dentries: "home", "user", "file.txt"
```

Dentry states: **used** (active reference), **unused** (no reference), **negative** (name does not exist — cached for performance).

### write() call path

```
User space: write()
    ↓
sys_write()
    ↓
VFS
    ↓
Filesystem-specific write method
    ↓
Block driver
    ↓
Storage medium
```

---

## 5. FUSE

**FUSE (Filesystem in Userspace)** allows implementing a filesystem in user space instead of the kernel.

```
Application
    ↓ POSIX syscalls
VFS
    ↓
FUSE kernel driver
    ↓ /dev/fuse
FUSE daemon (user space)
    ↓
Your filesystem code
```

### Advantages of user space

- Easier development and debugging.
- Crashes don't bring down the kernel.
- Use any language (Python, Go, Rust…).
- Portable across Linux, BSD, macOS.

### Disadvantage

- Lower performance than in-kernel implementations (context switches on every operation).

### Minimal operations

```c
static struct fuse_operations my_ops = {
    .getattr = my_getattr,
    .readdir = my_readdir,
    .open    = my_open,
    .read    = my_read,
};
```

```bash
./my_fs /mount/point    # mount
fusermount -u /mount/point  # unmount
```

---

## 6. DOS / FAT

Originally designed for MS-DOS floppy disks (360 KB). Now used in cameras, USB drives, embedded systems.

### Variants

- FAT12, FAT16, **FAT32** (most common).

### Disk layout

```
Reserved sectors (boot sector)
File Allocation Tables (×2)
Root directory
Data area
```

### File Allocation Table

A table with one entry per logical cluster. Each entry holds:
- 0: free cluster.
- Next cluster number: linked list of clusters forming a file.

A file is a chain of clusters:
```
Cluster 5 → Cluster 8 → Cluster 20 → EOF
```

### Directory entry (32 bytes)

| Bytes | Content |
|---|---|
| 0–7 | Filename (8.3 format) |
| 8–10 | Extension |
| 11 | Attributes |
| 22–23 | Time |
| 24–25 | Date |
| 26–27 | First cluster |
| 28–31 | File size |

### Strengths / Weaknesses

| Strength | Weakness |
|---|---|
| Simple | Fragmentation |
| Extremely portable | Limited security (no permissions) |
| Compatible with all OS | Not robust (no journaling) |

---

## 7. FFS — Fast File System

Origin: BSD, 1984. Also called UFS (Unix File System).

### Core idea

> Place related data and metadata close together on disk to minimize head movement.

### Structure

```
Boot blocks
Superblock
Cylinder groups:
    copy of superblock
    cylinder group header
    inode bitmap
    block bitmap
    inodes
    data blocks
```

### Inode block addressing

```
Inode
├── Direct blocks (data)
├── Single indirect → table of pointers → data
├── Double indirect → table → table → data
└── Triple indirect → table → table → table → data
```

### Soft Updates

Instead of synchronous writes, FFS tracks dependencies between metadata updates. Operations are applied in a safe order, ensuring consistency without needing a full journal.

Operations covered: file/directory creation and deletion, block allocation, indirect block management, free list management.

---

## 8. LFS — Log-Structured File System

Origin: 1990s, motivated by increasing RAM (more caching) and write-intensive workloads.

### Core idea

All writes (data + metadata) are buffered in memory and written as large sequential segments to disk.

```
Memory buffer → fill segment → write segment sequentially to disk
```

No in-place updates — old data is not erased immediately.

### Problem: locating inodes

Since inodes can be anywhere, LFS uses an **inode map** (indirection table).

### Problem: finding the inode map

A fixed **checkpoint region** stores the location of the current inode map (updated infrequently).

### Garbage collection

Old segments containing stale blocks must be reclaimed. A block is **live** if its inode still points to it.

**Hot segments** (frequently modified) → garbage collect first.  
**Cold segments** (stable data) → leave alone longer.

Main weakness: garbage collection is complex and adds overhead.

---

## 9. Ext2 / Ext3 / Ext4

### Evolution

| Version | Year | Key addition |
|---|---|---|
| **Ext** | 1992 | First Linux FS (improved Minix) |
| **Ext2** | 1993 | FFS-inspired, no journal |
| **Ext3** | ~2001 | Journaling, online resize, HTree directories |
| **Ext4** | ~2008 | Extents, large FS support, delayed allocation |

### Ext4 features

- **Extents**: instead of individual block pointers, use `(start_block, length)` pairs. More efficient for large files.
- **Files up to ~16 TiB**, **volumes up to ~1 EiB**.
- **HTree indexing**: B-tree-like index for large directories → O(log n) lookup.
- **Nanosecond timestamps**.
- **Metadata checksums**.

### Ext4 journaling modes

| Mode | What is journaled | Safety | Performance |
|---|---|---|---|
| `journal` | Data + metadata | Highest | Lowest |
| `ordered` (default) | Metadata only; data written first | Good | Good |
| `writeback` | Metadata only; no data ordering | Lower | Highest |

---

## 10. ZFS / OpenZFS

Origin: Sun Microsystems, 2001–2004. Combines volume manager and filesystem.

### Core principles

- **Never overwrite existing data** (copy-on-write).
- Transition from one consistent state to another.
- **Every block is checksummed** — detects silent corruption.
- Unified pool of storage shared by all filesystems and volumes.

### ZFS layers

```
ZPL (POSIX layer)  ← you interact here
ZAP (attribute processor: key-value directories)
DMU (data management unit: COW block management)
ZIL (intent log: synchronous write guarantees)
SPA (storage pool allocator)
RAIDZ / VDEV (virtual device layer)
ARC (adaptive replacement cache)
```

### Key features

- **Snapshots**: instant, space-efficient (CoW: unchanged blocks are shared).
- **Clones**: writable snapshots.
- **RAIDZ**: variable-stripe RAID without the write-hole problem.
- **ARC**: adaptive cache that uses available RAM efficiently.
- **Space maps**: track free space efficiently (not a bitmap but a log of allocations).

### RAIDZ reconstruction

After a disk failure, ZFS only reconstructs **used** data, not the entire disk.

### Availability

- Native on Solaris and FreeBSD.
- Linux: OpenZFS (licensed under CDDL, incompatible with GPL — not in the main kernel tree).
- macOS: OpenZFS available via third-party.

### Trade-offs

| Strength | Weakness |
|---|---|
| Strong integrity (checksums) | Needs 25%+ free space |
| Efficient snapshots/clones | High memory (ARC) |
| No write-hole in RAID | Linux GPL incompatibility |
| Intelligent reconstruction | Complex to configure |

---

## 11. NOVA

NOVA is a research filesystem designed for **persistent memory (PMEM)** — byte-addressable, fast, non-volatile memory (Intel Optane/3D XPoint).

### Key design choices

- Bypasses the block layer entirely — directly maps PMEM into kernel address space.
- **Per-inode log**: each inode has its own linked log of change entries.
- **Per-CPU allocators**: inode table and free-space table are partitioned by CPU → eliminates cross-CPU locking.

### Writing a file

1. Allocate memory from the CPU's free-space tree.
2. Copy data.
3. Append a log entry to the inode's log (describes the write).
4. Atomically update the log tail pointer.

### Copy-on-write

For overwrites:
1. Allocate new memory.
2. Copy old + new data.
3. Append new log entry.
4. Invalidate old entry.
5. Free old pages.

### Limitations

- x86-64 only.
- No ACL, no quotas, no `fsck`.
- CPU count must be the same when remounting on a different server.

---

## 12. LTFS

**LTFS (Linear Tape File System)** makes magnetic tapes usable as regular mounted filesystems.

### Problem solved

Traditional tape tools (`tar`, `cpio`) are not portable across systems. LTFS provides a standardized XML-based structure that works on Linux, Windows, and macOS.

### Tape structure

A tape volume has two partitions:
1. **Index partition**: XML metadata describing all files and directories.
2. **Data partition**: raw file data.

### Index format (XML)

```xml
<directory>
  <name>documents</name>
  <file>
    <name>report.pdf</name>
    <length>524288</length>
    <extentinfo>
      <extent>
        <startblock>1024</startblock>
        <byteoffset>0</byteoffset>
        <bytecount>524288</bytecount>
      </extent>
    </extentinfo>
  </file>
</directory>
```

Indexes are chained: each new index references the previous one → full history is recoverable.

### Strengths / Weaknesses

| Strength | Weakness |
|---|---|
| Portable across OS | Sequential access only |
| Mountable as a filesystem | Random access is very slow |
| SNIA standard | Tape seek = minutes |
| Good for archiving | Not for frequent updates |

---

## 13. Comparative Summary

| Filesystem | Target | Core idea | Strengths | Weaknesses |
|---|---|---|---|---|
| **FAT** | Floppy, USB | Linked cluster table | Simple, portable | Fragmentation, no permissions |
| **FFS** | HDD | Locality via cylinder groups | HDD-optimized, foundational ideas | Less relevant on modern storage |
| **LFS** | Write-heavy HDD | Sequential log | Great write throughput | Complex GC |
| **Ext2** | Linux | FFS-inspired | Simple, stable | No journal |
| **Ext3** | Linux | Ext2 + journal | Better reliability | Less modern than Ext4 |
| **Ext4** | General Linux | Extents + journal | Fast, robust, large scale | Increasing complexity |
| **ZFS** | Advanced storage | CoW + checksum + pool | Integrity, snapshots, RAID | Memory, license issues |
| **NOVA** | Persistent memory | Per-inode log, direct PMEM | PMEM-optimized | Research, many limitations |
| **LTFS** | Magnetic tape | XML index + sequential data | Portable archiving | No random access |

### When to use which

| Use case | Recommended filesystem |
|---|---|
| Maximum portability (USB, cameras) | FAT32 |
| General Linux system | Ext4 |
| Snapshots + strong integrity | ZFS/OpenZFS |
| Write-heavy workload on HDD | LFS-based system |
| Persistent memory (Optane) | NOVA |
| Long-term archiving on tape | LTFS |
| Prototype / custom filesystem | FUSE |
