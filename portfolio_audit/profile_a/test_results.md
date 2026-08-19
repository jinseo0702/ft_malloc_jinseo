# PROFILE A Baseline Results

## Summary

| Measure | Result | Classification |
|---|---:|---|
| Planned top-level cases | 30 | VERIFIED FROM CODE |
| Executed top-level cases | 30 | VERIFIED FROM RUNTIME TEST |
| PASS | 28 | VERIFIED FROM RUNTIME TEST |
| PARTIAL | 1 | VERIFIED FROM RUNTIME TEST |
| FAIL | 1 | VERIFIED FROM RUNTIME TEST |
| CRASH | 0 | VERIFIED FROM RUNTIME TEST |
| SETUP ERROR | 0 | VERIFIED FROM RUNTIME TEST |
| Source hashes unchanged | yes | VERIFIED FROM RUNTIME TEST |
| Harness mutations detected | 5 / 5 | VERIFIED FROM RUNTIME TEST |

The suite ran against an isolated build. No performance benchmark was run because correctness classification comes first.

## Case matrix

| ID | Result | Core observation |
|---|---|---|
| A01 | PASS | direct ft_malloc/write/free extent |
| A02 | PASS | minimal LD_PRELOAD malloc/free integration |
| A03 | PASS | zero-size result aligned and freeable |
| A04 | PASS | 1-byte TINY extent |
| A05 | PASS | 80-byte TINY boundary |
| A06 | PASS | 81-byte SMALL boundary |
| A07 | PASS | 496-byte SMALL boundary |
| A08 | PASS | 497-byte LARGE boundary |
| A09 | PASS | 6/6 representative sizes had remainder 0 at 16-byte `max_align_t` alignment |
| A10 | PASS | 221 calloc bytes zero and writable |
| A11 | PASS | overflowing calloc returned NULL |
| A12 | PASS | zero-operand calloc results aligned/freeable |
| A13 | PASS | free(NULL) completed |
| A14 | PASS | TINY first-free pointer reused |
| A15 | PASS | SMALL first-free pointer reused |
| A16 | PASS | 100 TINY allocations, 101st NULL, freed slot recovered |
| A17 | PASS | 100 SMALL allocations, 101st NULL, freed slot recovered |
| A18 | PASS | 100 LARGE allocations, 101st NULL, capacity recovered |
| A19 | PASS | realloc(NULL,64) returned writable extent |
| A20 | PASS | realloc(p,0) returned NULL and released slot |
| A21 | PASS | TINY in-zone prefix and identity preserved |
| A22 | PASS | TINY→SMALL prefix preserved and old TINY slot reusable |
| A23 | PASS | SMALL→LARGE 400-byte prefix preserved and old SMALL slot reusable |
| A24 | PASS | LARGE shrink prefix/extent and current identity policy |
| A25 | PASS | forced realloc failure left original data intact |
| A26 | PASS | 1000 sequential TINY/SMALL iterations used one 12288-byte and one 53248-byte arena mmap |
| A27 | PASS | 10 LARGE allocations produced 10 matching mmap/munmap pairs |
| A28 | FAIL | `show_alloc_mem` omitted live allocation data and reported wrong totals |
| A29 | PASS | 4 threads × 500 operations completed with integrity=1 |
| A30 | PARTIAL (NON-CONTRACT) | forged foreign metadata cleared a live slot; double free was not detected |

## Important PASS evidence

- **Boundary/extent:** 1, 80, 81, 496, and 497-byte cases all returned writable extents, covering both exact class boundaries.
- **Alignment:** sizes 1, 80, 81, 496, 497, and 4097 all had address remainder 0 for the measured 16-byte `max_align_t` alignment.
- **Capacity/recovery:** all three classes admitted their code-defined first 100 simultaneous allocations, rejected the 101st, and recovered after one free.
- **Realloc:** null, zero, in-zone, cross-zone, shrink, and forced-failure paths preserved their defined invariants in A19–A25.
- **Resource lifecycle:** A26 observed exactly one calculated TINY and SMALL arena mapping; A27 matched all 10 LARGE mappings to successful unmaps.
- **Concurrency:** 2000 total controlled operations across four threads completed without crash, timeout, allocation failure, or detected data corruption.

## Environment retry distinction

The first A26/A27 strace attempts were denied by the execution sandbox (`PTRACE_TRACEME` / `PTRACE_SEIZE: Operation not permitted`). They were not target failures. Both were rerun with approved strace permission and passed. The initial stderr is superseded by the successful raw traces, while retry metadata remains in `test_results.json`.

## Harness validation

Controlled wrong evidence was injected for alignment, calloc zeroing, TINY reuse, realloc preservation, and LARGE unmap pairing. The corrected validator detected all 5/5 as FAIL.

The first mutation attempt detected 4/5 because its A27 mutation removed a startup `munmap` outside the marked workload. That mutation-builder error was preserved in `harness_validation.initial.json`; after targeting the first post-marker `munmap`, validation reached 5/5. No target execution was changed during mutation validation.

## Raw evidence

- machine-readable aggregate: `test_results.json`
- per-case stdout/stderr/result: `raw/run_001/cases/A01` through `A30`
- syscall traces: `raw/run_001/cases/A26/strace.raw`, `A27/strace.raw`
- repeat verification: `raw/run_001/repeats/`
- mutation evidence: `raw/run_001/harness_validation_retry/`
