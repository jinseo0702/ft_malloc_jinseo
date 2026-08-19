# PROFILE A Baseline

Recorded on 2026-08-19 (Asia/Seoul).

| Field | Value | Classification |
|---|---|---|
| Commit | `2230e474cd78911ecf08f931fcae09e8100dfc39` | VERIFIED FROM CODE |
| Branch | `main` tracking `origin/main` | VERIFIED FROM CODE |
| Pre-PROFILE-A dirty state | only untracked `portfolio_audit/` from the prior audit; tracked source clean | VERIFIED FROM CODE |
| OS / kernel | Ubuntu 26.04 LTS / Linux `7.0.0-29-generic` | VERIFIED FROM RUNTIME TEST |
| Architecture | x86-64, 64-bit userspace | VERIFIED FROM RUNTIME TEST |
| Compiler | GCC `15.2.0` | VERIFIED FROM RUNTIME TEST |
| libc diagnostic reference | glibc `2.43` | VERIFIED FROM RUNTIME TEST |
| Page size | 4096 bytes | VERIFIED FROM RUNTIME TEST (`getconf PAGE_SIZE`) |
| syscall tracer | strace `6.19` | VERIFIED FROM RUNTIME TEST |
| Valgrind | unavailable (`command not found`) | VERIFIED FROM RUNTIME TEST |
| Build commands | `make`; direct-test object build derived from root Makefile inputs; optional project target `make static` | VERIFIED FROM CODE |
| Production run mode | `LD_PRELOAD=./libft_malloc_x86_64_Linux.so <workload>` | VERIFIED FROM CODE |

The default `make` build has already succeeded in an isolated repository copy and produced an ELF64 shared object. This is build evidence only, not allocator correctness evidence.

## Source integrity

PROFILE A started with the same hashes recorded before the PROFILE B mismatch campaign:

```text
18d485f79c2d3a70380c02b728e7ad1b2364625f1392df69adf7f0d79c7cbfd4  Makefile
9ac3a3082524ef8f83bc7529d759ebb91195675b3b7b752c7ad50eb627b90dd8  README.md
625485d579e957a3b642b3ebade048cf197de752618e5d82c3fcca43f5ab4fff  include/ft_malloc.h
98b938353cc1fa40261d3750b35c65d4b31b0c199192c8066449f4503ecc5bee  src/ft_malloc.c
9dc55534d2cec1f2a1cb39577e0650d96b6b16cec53224d08085cbaf0aee4fe9  src/ft_malloc_ver.0.1.c
07c6f7aca2757cd9f725cc1d28476416f682b7306ac460ff2d66444445047209  src/ft_override.c
```

All PROFILE A tests will build a fresh source copy under `/tmp`; only harnesses, raw logs, and reports may be written below `portfolio_audit/profile_a/`.
