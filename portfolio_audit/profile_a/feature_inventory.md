# Allocator Feature Inventory

`IMPLEMENTED` means a current code path exists, not that its semantics have passed runtime tests.

| Feature | Status | Source Evidence | Runtime Verified? |
|---|---|---|---|
| Shared-library build for host architecture/OS | IMPLEMENTED | `Makefile:1-30` | Build only |
| `malloc` / `ft_malloc` | IMPLEMENTED | `src/ft_override.c:3-5`; `src/ft_malloc.c:373-378,630-723` | No |
| `free` / `ft_free` | IMPLEMENTED | `src/ft_override.c:16-18`; `src/ft_malloc.c:380-384,727-765` | No |
| `realloc` / `ft_realloc` | IMPLEMENTED | `src/ft_override.c:7-9`; `src/ft_malloc.c:386-391,808-857` | No |
| `calloc` / `ft_calloc2` | IMPLEMENTED | `src/ft_override.c:11-13`; `src/ft_malloc.c:393-398,859-874` | No |
| Zero-size malloc normalization to one byte | IMPLEMENTED | `src/ft_malloc.c:634-637` | No |
| Calloc overflow rejection | IMPLEMENTED | `src/ft_malloc.c:863-867` | No |
| TINY fixed slots and bitmap reuse | IMPLEMENTED | `src/ft_malloc.c:534-545,584-605,651-675` | No |
| SMALL fixed slots and bitmap reuse | IMPLEMENTED | `src/ft_malloc.c:546-556,606-625,676-700` | No |
| LARGE direct mmap/munmap | IMPLEMENTED | `src/ft_malloc.c:701-720,750-760` | No |
| Realloc null/zero cases | IMPLEMENTED | `src/ft_malloc.c:813-819` | No |
| Realloc prefix-copy path | IMPLEMENTED | `src/ft_malloc.c:831-851` | No |
| One global allocator mutex | IMPLEMENTED | `src/ft_malloc.c:368-398`; `include/ft_malloc.h:76-82` | No |
| `show_alloc_mem` diagnostics | IMPLEMENTED | `src/ft_malloc.c:400-445` | No |
| `free(NULL)` | IMPLEMENTED | `src/ft_malloc.c:727-732` | No |
| Non-null invalid-pointer ownership validation | PARTIAL | pointer is treated as having preceding `METADATA_t`; no arena/table ownership check: `src/ft_malloc.c:733-764` | No |
| `aligned_alloc`, `posix_memalign`, `memalign`, `valloc`, `pvalloc` | NOT IMPLEMENTED | no declarations, overrides, or active implementation paths | No |

## Functional scope

**VERIFIED FROM CODE:**

- 4 standard allocation names are overridden and 4 corresponding `ft_*` APIs are exposed.
- 3 allocation classes are implemented.
- TINY: 100 slots, 96-byte stride, intended payload range 1–80 bytes.
- SMALL: 100 slots, 512-byte stride, intended payload range 81–496 bytes.
- LARGE: requests above 496 bytes, with at most 100 concurrent table entries.
- TINY/SMALL policy is first-free fixed-slot reuse; split and coalesce are not part of the implemented model.
- Fixed arenas are retained; LARGE mappings are released individually.

These are implementation-scope numbers. They do not establish semantic correctness, ABI alignment, leak freedom, or efficiency.
