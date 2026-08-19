# Allocator Architecture

## Control flow

```text
Host allocation call or direct ft_* call
        ↓
malloc/free/realloc/calloc interposition (`src/ft_override.c`)
        ↓
ft_malloc / ft_free / ft_realloc / ft_calloc2
        ↓ acquire one global pthread mutex
malloc_impl / free_impl / realloc_impl / callo_imp
        ↓
request-size policy
  ├─ 1–80 B   → TINY: 100 × 96-byte fixed slots + bitmap
  ├─ 81–496 B → SMALL: 100 × 512-byte fixed slots + bitmap
  └─ >496 B   → LARGE: one page-rounded mmap per allocation
        ↓
[METADATA_t][client payload]
        ↓
free: clear fixed-slot bit or munmap LARGE mapping
```

## Components

| Component | Behavior | Evidence | Classification |
|---|---|---|---|
| Interposition layer | libc allocation names forward to `ft_*` APIs | `src/ft_override.c:3-18` | VERIFIED FROM CODE |
| API lock boundary | four `ft_*` APIs acquire one global mutex and call no-lock implementations | `src/ft_malloc.c:368-398`; `include/ft_malloc.h:76-82` | VERIFIED FROM CODE |
| TINY arena | lazy page-rounded mmap, 100 metadata/slot records, first-free bitmap claim | `src/ft_malloc.c:484-507,534-545,584-605,651-675` | VERIFIED FROM CODE |
| SMALL arena | lazy page-rounded mmap, 100 metadata/slot records, first-free bitmap claim | `src/ft_malloc.c:484-507,546-556,606-625,676-700` | VERIFIED FROM CODE |
| LARGE mapping | page-rounded mmap for each allocation, address table and count bounded by 100 | `src/ft_malloc.c:557-569,701-720` | VERIFIED FROM CODE |
| Inline metadata | mapped size/index, bit position, and client-request size precede payload | `include/ft_malloc.h:37-41`; `src/ft_malloc.c:534-569` | VERIFIED FROM CODE |
| Free lifecycle | fixed slots become reusable; LARGE entries are removed and unmapped | `src/ft_malloc.c:727-765` | VERIFIED FROM CODE |
| Realloc lifecycle | no-lock allocate/copy/free composition with selected in-place cases | `src/ft_malloc.c:808-857` | VERIFIED FROM CODE |
| Calloc lifecycle | multiplication-overflow check, allocation, zero fill | `src/ft_malloc.c:859-874` | VERIFIED FROM CODE |
| Diagnostics | traverses allocation state and prints per-zone entries and total requested bytes | `src/ft_malloc.c:400-445` | VERIFIED FROM CODE |

## Important design characteristics

- **VERIFIED FROM CODE — segregated fixed-slot policy:** TINY and SMALL avoid per-allocation mapping after lazy arena creation; neither uses split/coalesce because slots are fixed-size.
- **VERIFIED FROM CODE — direct LARGE lifecycle:** requests above 496 bytes map independently and are individually unmapped.
- **VERIFIED FROM CODE — first-free reuse:** bitmap scan order selects the lowest free slot.
- **VERIFIED FROM CODE — bounded capacity:** 100 TINY, 100 SMALL, and 100 concurrently tracked LARGE allocations.
- **VERIFIED FROM CODE — coarse thread serialization:** one mutex covers each public allocator transaction, while internal helpers avoid nested locking during realloc/calloc.
- **VERIFIED FROM CODE — asymmetric retention:** fixed arenas remain mapped after all slots are freed; LARGE mappings do not.
- **UNKNOWN — runtime invariants:** alignment, boundary correctness, data preservation, concurrency safety, and mapping balance remain unverified until the approved suite runs.
- **INFERENCE — trade-off:** fixed slots simplify constant-size reuse but impose hard capacity and predictable internal fragmentation; performance and memory-efficiency claims require measurement and are not made here.
