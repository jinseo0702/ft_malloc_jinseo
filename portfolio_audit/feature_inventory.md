# Feature Inventory

`IMPLEMENTED` below means an active code path exists. It does not claim semantic correctness. `Runtime Verified?` is `Build only` when the isolated compilation reached the shared-library artifact but the behavior itself was not exercised.

## Actual repository features

| Feature | Status | Source Evidence | Runtime Verified? |
|---|---|---|---|
| Host-specific shared-library build | IMPLEMENTED | `Makefile:1-30` | Yes — isolated build only |
| `LD_PRELOAD` overrides for `malloc`, `free`, `realloc`, `calloc` | IMPLEMENTED | `src/ft_override.c:3-18` | Build only |
| Public `ft_malloc`, `ft_free`, `ft_realloc`, `ft_calloc2` API | IMPLEMENTED | `include/ft_malloc.h:62-69`; `src/ft_malloc.c:373-398` | Build only |
| TINY fixed-slot allocation for requests 1–80 bytes | IMPLEMENTED | `include/ft_malloc.h:20-24`; `src/ft_malloc.c:651-675` | No |
| SMALL fixed-slot allocation for requests 81–496 bytes | IMPLEMENTED | `include/ft_malloc.h:20-24`; `src/ft_malloc.c:676-700` | No |
| LARGE direct mapping for requests above 496 bytes | IMPLEMENTED | `src/ft_malloc.c:701-720` | No |
| TINY/SMALL first-free bitmap slot selection | IMPLEMENTED | `src/ft_malloc.c:584-625` | No |
| TINY/SMALL slot reuse after free | IMPLEMENTED | allocation: `src/ft_malloc.c:584-625`; release: `src/ft_malloc.c:740-748` | No |
| LARGE unmap on free | IMPLEMENTED | `src/ft_malloc.c:750-760` | No |
| `realloc(NULL,n)` and `realloc(p,0)` paths | IMPLEMENTED | `src/ft_malloc.c:813-819` | No |
| Cross-zone realloc copy using `min(old_size,new_size)` | IMPLEMENTED | `src/ft_malloc.c:826-851` | No |
| Calloc multiplication-overflow guard and zero fill | IMPLEMENTED | `src/ft_malloc.c:859-874` | No |
| One global mutex around public allocation APIs | IMPLEMENTED | `src/ft_malloc.c:368-398`; `include/ft_malloc.h:76-82` | No |
| Allocation-state display | IMPLEMENTED | `src/ft_malloc.c:400-445` | No |
| Null `free` handling | IMPLEMENTED | `src/ft_malloc.c:727-732` | No |
| General invalid-pointer validation | PARTIAL | null is handled, but non-null input is dereferenced as preceding metadata without an ownership/range check: `src/ft_malloc.c:733-764` | No |

## Current functional scope

**VERIFIED FROM CODE:**

- 4 libc allocation names are overridden: `malloc`, `free`, `realloc`, `calloc`.
- 4 corresponding `ft_*` public allocation functions are declared and implemented.
- 3 allocation classes exist: TINY, SMALL, LARGE.
- TINY has 100 managed slots with a 96-byte stride and an intended maximum payload of 80 bytes.
- SMALL has 100 managed slots with a 512-byte stride and an intended maximum payload of 496 bytes.
- At most 100 LARGE mappings are admitted concurrently by `large_cnt < 100`.
- TINY/SMALL use first-free bitmap reuse; there is no split or coalesce path because each slot has a fixed size.

These figures describe implemented capacity and code paths, not proven correctness or measured memory efficiency.

## PROFILE B applicability inventory

A repository-wide search of tracked C headers/sources, Markdown, and Makefiles found no production ELF/symbol/nm implementation. Test programs have `main` functions, but they exercise allocator/support behavior rather than parse ELF inputs.

| PROFILE B capability | Status | Source Evidence | Runtime Verified? |
|---|---|---|---|
| Production command accepting an ELF/archive path | NOT IMPLEMENTED | root target is a shared library; no production `main(argc,argv)` | Build confirms a shared object, not a CLI |
| ELF header/program/section parsing | NOT IMPLEMENTED | no ELF parsing types, constants, or code paths found | No |
| `.symtab` / `.dynsym` parsing | NOT IMPLEMENTED | no symbol-table parser found | No |
| Symbol-name extraction | NOT IMPLEMENTED | no string-table/symbol parser found | No |
| Symbol-value extraction | NOT IMPLEMENTED | no symbol-record output path found | No |
| GNU nm-style symbol-type classification | NOT IMPLEMENTED | no ELF binding/type/section classifier found | No |
| Symbol inclusion/exclusion policy | NOT IMPLEMENTED | no symbol iteration/filtering path found | No |
| Symbol ordering | NOT IMPLEMENTED | no symbol collection/sort/output path found | No |
| Relocatable object input | NOT IMPLEMENTED | no file-input parser | No |
| Executable input | NOT IMPLEMENTED | no file-input parser | No |
| Shared-object input | NOT IMPLEMENTED | no file-input parser | No |
| Static-archive/member input | NOT IMPLEMENTED | no archive parser | No |
| Stripped-binary behavior | NOT IMPLEMENTED | no ELF parser/error path | No |
| Malformed-input error behavior | NOT IMPLEMENTED | no target file-input path | No |
| nm-compatible command-line options | NOT IMPLEMENTED | no production option parser | No |

Therefore the current PROFILE B functional scope is **0 production ELF-input commands, 0 nm-compatible options, and 0 implemented symbol-output fields** (name/value/type/inclusion-order output are absent). This is a code-presence count, not a runtime correctness score.

The isolated build artifact is itself an ELF shared object, and GNU nm can list its exported symbols. That observation belongs to artifact inspection; it is not evidence of target-side parsing.
