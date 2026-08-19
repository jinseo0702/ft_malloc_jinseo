# Repository Architecture Audit

## Product identity

**VERIFIED FROM CODE:** The production target is a shared library that interposes the C allocation functions through `LD_PRELOAD`. It is not an ELF symbol parser and has no production executable entry point.

## Control flow

```text
Host process allocation call
  malloc / free / realloc / calloc
                 ↓
src/ft_override.c interposition layer
                 ↓
ft_malloc / ft_free / ft_realloc / ft_calloc2
  acquire one global pthread mutex
                 ↓
malloc_impl / free_impl / realloc_impl / callo_imp
                 ↓
size classification and shared allocator state
      ┌──────────────┼──────────────┐
      ↓              ↓              ↓
TINY ≤ 80 B     SMALL ≤ 496 B    LARGE > 496 B
96 B slots      512 B slots      per-allocation mapping
100 slots       100 slots        table capped at 100
bitmap          bitmap           large[101] + large_cnt
      └──────────────┼──────────────┘
                     ↓
inline METADATA_t immediately before client payload
                     ↓
mmap arena/direct mapping → returned payload pointer
free: bitmap clear or LARGE munmap
```

There is no PROFILE B flow of `ELF input → ELF parser → symbol classifier → ordered nm output` in the production code.

## Components and evidence

| Component | Responsibility | Evidence | Classification |
|---|---|---|---|
| Root Makefile | Builds a host-specific shared library from `src/ft_malloc.c` and `src/ft_override.c`; also offers a static target | `Makefile:1-30` | VERIFIED FROM CODE |
| Override layer | Maps `malloc/free/realloc/calloc` to the `ft_*` public API | `src/ft_override.c:3-18` | VERIFIED FROM CODE |
| Locked public API | Serializes each public allocation operation with one global mutex, then invokes a no-lock internal implementation | `src/ft_malloc.c:368-398`; `include/ft_malloc.h:76-82` | VERIFIED FROM CODE |
| Zone allocator | Chooses TINY, SMALL, or LARGE; lazily maps memory; writes client size; returns `metadata + 1` | `src/ft_malloc.c:630-723` | VERIFIED FROM CODE |
| Fixed-slot metadata | Stores mapped size/bitmap index, bit position, and requested client size in a declared 16-byte metadata layout | `include/ft_malloc.h:20-47`; `src/ft_malloc.c:534-573` | VERIFIED FROM CODE; actual ABI size/alignment remains runtime-unverified |
| Free-list representation | Uses two 16-byte bitmaps but scans only the first 100 slot positions | `include/ft_malloc.h:43-49`; `src/ft_malloc.c:584-625` | VERIFIED FROM CODE |
| Resource release | TINY/SMALL clears a bitmap bit; LARGE clears its table entry and calls `munmap` | `src/ft_malloc.c:727-765` | VERIFIED FROM CODE |
| Reallocation | Handles null/zero, reuses a block for selected zone relations, otherwise allocates, copies `min(old,new)`, then frees the old block | `src/ft_malloc.c:808-857` | VERIFIED FROM CODE; semantic correctness not runtime-verified |
| Zeroed allocation | Checks multiplication overflow, allocates, then zero-fills; zero operands route to a minimum allocation | `src/ft_malloc.c:859-874` | VERIFIED FROM CODE; semantic correctness not runtime-verified |
| Diagnostics | Prints current TINY/SMALL/LARGE allocations and total requested bytes | `src/ft_malloc.c:400-445` | VERIFIED FROM CODE |
| System interface | Uses `pthread_mutex_*`, `sysconf`, `mmap`, `munmap`, error/stdio helpers | `include/ft_malloc.h`; `src/ft_malloc.c:447-507,630-765` | VERIFIED FROM CODE |

## Data and resource lifecycle

1. The first allocation in TINY or SMALL calculates a page-rounded arena size, maps storage, and initializes 100 inline metadata records.
2. A bitmap scan claims the first free slot. The requested size is written into that slot's metadata and the payload address is returned.
3. Freeing a TINY/SMALL allocation clears its bitmap bit. The arena itself is not unmapped by current code.
4. Each LARGE allocation is separately page-rounded and mapped, recorded in `large[101]`, and counted by `large_cnt`.
5. Freeing a LARGE allocation clears the table entry, calls `munmap`, and decrements `large_cnt`.

All five statements are **VERIFIED FROM CODE**. Their correctness under malformed pointers, concurrency, allocation failure, and repeated workloads is **UNKNOWN** until runtime tests are approved and run.

## Important design characteristics

- **VERIFIED FROM CODE — interposition boundary:** the libc-compatible names are a thin override layer; allocator logic lives behind explicit `ft_*` functions.
- **VERIFIED FROM CODE — segregated policy:** fixed 96-byte and 512-byte slots handle requests up to 80 and 496 payload bytes; larger requests use direct mappings.
- **VERIFIED FROM CODE — inline metadata:** the client pointer follows allocator metadata, so free/realloc recover state by subtracting one `METADATA_t`.
- **VERIFIED FROM CODE — bounded state:** current scans and counters cap TINY, SMALL, and simultaneously tracked LARGE allocations at 100 each.
- **VERIFIED FROM CODE — coarse locking:** a single process-global mutex serializes the four public `ft_*` allocation operations; internal composite operations call no-lock helpers.
- **VERIFIED FROM CODE — mixed release policy:** fixed arenas persist after slots are freed, while LARGE mappings are individually released.
- **VERIFIED FROM CODE — fail modes differ:** several allocation failures return `NULL`, while page-size, mutex, or `munmap` errors call `print_error`, which terminates the process.
- **INFERENCE — design trade-off:** fixed slots and a bitmap simplify lookup and reuse at the cost of hard capacity and internal fragmentation. This is an inference from structure, not user-provided rationale or measured performance.
