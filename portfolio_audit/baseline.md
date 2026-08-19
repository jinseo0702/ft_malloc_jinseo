# Baseline Environment

Recorded on 2026-08-19 (Asia/Seoul).

| Field | Value | Classification |
|---|---|---|
| Commit | `2230e474cd78911ecf08f931fcae09e8100dfc39` | VERIFIED FROM CODE (`git rev-parse HEAD`) |
| Branch | `main` tracking `origin/main` | VERIFIED FROM CODE |
| Dirty before audit | clean | VERIFIED FROM CODE (`git status --short --branch` contained no path entries) |
| OS | Ubuntu 26.04 LTS (Resolute Raccoon) | VERIFIED FROM RUNTIME TEST |
| Kernel | Linux `7.0.0-29-generic` | VERIFIED FROM RUNTIME TEST |
| Architecture | `x86_64` | VERIFIED FROM RUNTIME TEST |
| Compiler | GCC `15.2.0` (`cc`; Makefile selects `gcc`) | VERIFIED FROM RUNTIME TEST / VERIFIED FROM CODE |
| Build tool | GNU Make `4.4.1` | VERIFIED FROM RUNTIME TEST |
| PROFILE B primary reference | GNU nm (GNU Binutils for Ubuntu) `2.46` | VERIFIED FROM RUNTIME TEST |
| Build command | `make` | VERIFIED FROM CODE (`README.md`, root `Makefile`) |
| Produced artifact | `libft_malloc_x86_64_Linux.so` | VERIFIED FROM RUNTIME TEST in an isolated copy |
| Artifact format | ELF64 LSB x86-64 shared object (`ET_DYN`), dynamically linked, not stripped | VERIFIED FROM RUNTIME TEST (`file`, `readelf -h`) |
| Allocator run mode | `LD_PRELOAD=./libft_malloc_x86_64_Linux.so <program>` | VERIFIED FROM CODE (`README.md`, override layer) |
| PROFILE B target run command | none | VERIFIED FROM CODE; no production `main`, ELF-input parser, or nm-style CLI exists |

## Isolated build

`make` completed with exit code 0 in a repository copy under `/tmp/ft_malloc_checkpoint_a.n1KG5C/repo`. The original checkout was not built and received no object, archive, or shared-library artifacts.

This build verifies only that the current source compiles into a shared library in this environment. It does **not** verify allocator correctness and it does **not** establish PROFILE B compatibility.

Running GNU `nm -D --defined-only` on the isolated shared library showed allocator and support exports including `malloc`, `free`, `realloc`, `calloc`, `ft_malloc`, `ft_free`, `ft_realloc`, and `ft_calloc2`. This proves that GNU nm can inspect the built library as an ELF input; it does not turn the library itself into an nm implementation.

## Pre-audit integrity hashes

The active implementation inputs were hashed before any audit artifact was written:

```text
18d485f79c2d3a70380c02b728e7ad1b2364625f1392df69adf7f0d79c7cbfd4  Makefile
9ac3a3082524ef8f83bc7529d759ebb91195675b3b7b752c7ad50eb627b90dd8  README.md
625485d579e957a3b642b3ebade048cf197de752618e5d82c3fcca43f5ab4fff  include/ft_malloc.h
98b938353cc1fa40261d3750b35c65d4b31b0c199192c8066449f4503ecc5bee  src/ft_malloc.c
9dc55534d2cec1f2a1cb39577e0650d96b6b16cec53224d08085cbaf0aee4fe9  src/ft_malloc_ver.0.1.c
07c6f7aca2757cd9f725cc1d28476416f682b7306ac460ff2d66444445047209  src/ft_override.c
264df5189a6ad0ab43ee9a378c5b0be9bc57be1cebc283cc1d165e91080ff7d5  libft/Makefile
146343a6055fd737a92fd258863e7a286e6136e8958bef163c8c9b16d8352229  printf/Makefile
f5e0259f37b9a7865009ed59c0587ed7d9b8e0aebb718ae25ed9d161f37c8db0  test/Makefile
```

`src/ft_malloc_ver.0.1.c` is tracked historical/alternate source but is not listed in the root Makefile's `SRC` variable. The active build compiles `src/ft_malloc.c` and `src/ft_override.c`.

A post-write integrity check reproduced every hash above. `git diff` showed no tracked source change, no `.o`, `.a`, or `.so` artifact appeared in the original checkout, and `git status --short` reported only the new untracked `portfolio_audit/` directory. **VERIFIED FROM RUNTIME TEST.**
