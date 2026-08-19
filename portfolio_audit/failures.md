# Failure Analysis

## Semantic failures

None. PROFILE B semantic tests executed: 0; FAIL: 0; PARTIAL: 0; CRASH: 0.

This is not evidence that the allocator is correct. The requested test profile could not address allocator semantics.

## Applicability finding

| Field | Finding |
|---|---|
| Expected by PROFILE B | A target command that consumes ELF/archive input and emits nm-style symbol records |
| Actual repository product | A shared library that interposes allocation APIs and consumes allocation calls |
| Relevant code/build path | `src/ft_override.c` → `ft_*` API → allocator implementation; root `make` produces `libft_malloc_x86_64_Linux.so` |
| Runtime evidence | isolated build exit 0; artifact is an ELF64 shared object; no target command exists |
| Classification | scope/applicability mismatch |
| Correctness-bug status | not established |
| Severity to PROFILE B campaign | blocking: no semantic comparison interface |

The shared library being an ELF file is not a substitute for an nm implementation. GNU nm can inspect that artifact, but there is no independent target output to compare.

## Possible directions

1. Apply PROFILE A to `ft_malloc_jinseo` and test allocator API semantics, memory behavior, and resource lifecycle.
2. Apply PROFILE B to `gnu_nm_project`, which is the repository named by the profile.
3. Add a separate ELF/nm executable to this repository only if combining unrelated products is an intentional requirement. This would be new feature work and is outside the approved source-preserving audit.

Direction 1 or 2 preserves a coherent portfolio narrative. Direction 3 is listed for completeness, not recommended from current evidence.
