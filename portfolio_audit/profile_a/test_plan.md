# PROFILE A Baseline Test Plan

This plan was approved and executed as `run_001`. Results are recorded in `test_results.md` and `test_results.json`.

## Test isolation and build strategy

1. Recheck source hashes before and after the campaign.
2. Copy the project to a fresh `/tmp/ft_malloc_profile_a.*` directory and build only there.
3. Build a **direct API test target** from active `ft_malloc` implementation/support objects without `src/ft_override.o`, so libc's own internal allocations cannot consume the target's 100-slot tables.
4. Build the normal shared library separately for explicit `LD_PRELOAD` integration cases.
5. Run every case in a fresh subprocess with a fixed timeout, capturing stdout, stderr, exit status, signal, fixture hash, command, and environment.
6. Store harnesses under `portfolio_a/tests/`, tools under `portfolio_a/tools/`, binaries under `portfolio_a/bin/`, and unmodified outputs under `portfolio_a/raw/`.

Build/setup failures are not target failures and remain outside PASS/PARTIAL/FAIL/CRASH counts.

## Oracle hierarchy

1. **Primary — C allocation semantics and observable invariants:** null/success result, usable requested extent, alignment for `max_align_t`, calloc zeroing/overflow, free-null behavior, realloc prefix preservation, and failure preservation.
2. **Project-specific code invariant:** exact zone boundaries, 100-slot capacities, first-free address reuse, fixed-arena retention, and LARGE mapping release. These are scored as implementation-policy tests, not requirements imposed by libc.
3. **System-call oracle:** strace 6.19 records `mmap`/`munmap` lifecycle; page-rounded expected lengths are calculated from runtime page size rather than hard-coded.
4. **Relative diagnostic only:** glibc 2.43 may run API-compatible workloads to validate fixture logic, but pointer identity, capacity, syscall counts, memory use, and speed are not expected to match.

`malloc(0)` and zero-operand calloc accept either `NULL` or a suitably aligned freeable pointer under the general C contract; the harness never dereferences a zero-size result. `ft_realloc(p,0)` is tested against this project's explicit free-and-NULL code path and slot-reuse invariant, not used as a cross-libc portability claim.

Non-null invalid free and double free are outside the C API contract. Their outcomes are recorded as robustness/crash observations and must not be described as allocator correctness failures without a project-specific safety promise.

## Deterministic baseline suite — 30 top-level cases

| ID | Case | Oracle / invariant |
|---|---|---|
| A01 | direct `ft_malloc(32)` → write 32 bytes → `ft_free` | non-null, writable extent, clean exit |
| A02 | minimal libc `malloc/free` workload under `LD_PRELOAD` | override loads, extent writable, clean exit |
| A03 | `ft_malloc(0)` | C-allowed zero-size result; any non-null result aligned and freeable |
| A04 | 1-byte allocation | TINY success and one-byte write/read |
| A05 | 80-byte boundary | TINY success and full-extent pattern integrity |
| A06 | 81-byte boundary | SMALL success and full-extent pattern integrity |
| A07 | 496-byte boundary | SMALL success and full-extent pattern integrity |
| A08 | 497-byte boundary | LARGE success and full-extent pattern integrity |
| A09 | alignment at sizes `{1,80,81,496,497,4097}` | every non-null result divisible by runtime `_Alignof(max_align_t)` |
| A10 | `ft_calloc2(17,13)` | 221 bytes initially zero and writable |
| A11 | overflowing calloc multiplication | returns `NULL`; no target allocation becomes visible |
| A12 | zero-operand calloc | C-allowed zero-size result; non-null result aligned/freeable |
| A13 | `ft_free(NULL)` | no effect and clean exit |
| A14 | TINY allocate/free/allocate | first freed slot address reused |
| A15 | SMALL allocate/free/allocate | first freed slot address reused |
| A16 | TINY capacity and recovery | first 100 simultaneous allocations succeed, 101st returns `NULL`, freeing one permits one allocation at that address |
| A17 | SMALL capacity and recovery | same 100/101/recovery invariant |
| A18 | LARGE capacity and recovery | first 100 simultaneous mappings succeed, 101st returns `NULL`, freeing one permits another allocation |
| A19 | `ft_realloc(NULL,64)` | equivalent observable result to target malloc path; writable 64-byte extent |
| A20 | `ft_realloc(p,0)` | returns `NULL`, releases original slot, next same-zone allocation can reuse it |
| A21 | TINY in-zone realloc | prefix preserved; code-policy in-place identity checked separately |
| A22 | TINY → SMALL realloc | prefix `min(old,new)` preserved; old slot becomes reusable |
| A23 | SMALL → LARGE realloc | prefix preserved; old SMALL slot becomes reusable |
| A24 | LARGE → smaller realloc | prefix preserved; returned extent usable; code-policy pointer identity recorded |
| A25 | forced realloc allocation failure | returns `NULL` and original allocation/pattern remains valid and freeable |
| A26 | repeated sequential TINY/SMALL reuse | after lazy initialization, no unbounded additional arena-sized `mmap`; calculated arena lengths/counts preserved |
| A27 | repeated LARGE allocate/free lifecycle | each successful target mapping has a corresponding successful `munmap` of the same normalized mapping/length |
| A28 | `show_alloc_mem` before and after representative frees | zone inclusion and total requested-byte accounting match live allocations |
| A29 | 4-thread controlled integrity workload | no crash/timeout; every successful extent retains thread pattern; target operations complete |
| A30 | non-contract robustness matrix: foreign pointer and double free, each in its own subprocess | record exit/signal/timeout and code path; do not infer C-contract failure |

The suite contains exactly 30 top-level cases. A30 has two explicitly labeled subprocess observations but remains one robustness case in result counts.

## Resource and mapping measurements

Expected fixed-arena mapping lengths are computed as:

```text
round_up(96 × 100, runtime_page_size)
round_up(512 × 100, runtime_page_size)
```

At the measured 4096-byte page size these evaluate to 12288 and 53248 bytes, but the harness records and uses the calculation rather than assuming those constants. Marked `write` events delimit the workload region in strace output, preventing startup mappings from being attributed to the allocator case.

Fixed-arena retention after all slots are freed is treated as the implemented policy. LARGE mapping imbalance is a resource-lifecycle failure. RSS and benchmark throughput are intentionally excluded from this baseline because they are noisy or performance-oriented; performance testing starts only after correctness classification.

## Normalization

Normalized:

- ASLR-dependent addresses become stable symbolic IDs such as `<TINY_SLOT_0>` or `<MAP_1>` after equality, ordering, range, and alignment relations are evaluated;
- strace PID/timestamp prefixes are removed, while syscall name, order, length, protection/flags, result class, errno, and mapping-pair relationships remain;
- absolute `/tmp` campaign roots become `<AUDIT_TMP>` in display copies; raw output remains untouched;
- thread log ordering is ignored, while per-thread completion counts and data-integrity assertions remain;
- `show_alloc_mem` pointer text becomes symbolic addresses only after live-allocation membership is checked.

Not normalized:

- null versus non-null result;
- alignment remainder;
- requested-size writability or preserved data bytes;
- pointer equality/reuse relationships;
- TINY/SMALL/LARGE boundary selection;
- allocation capacity and recovery counts;
- mmap/munmap count, order, length, flags, success/failure, or pairing;
- diagnostic zone membership, requested sizes, or total;
- exit status, signal, crash, timeout, or deadlock;
- calloc zero content or overflow result.

## Classification rules

- `PASS`: all core semantic or explicitly labeled project-policy assertions pass.
- `PARTIAL`: usable core behavior exists, but a meaningful promised sub-behavior or diagnostic field is missing.
- `FAIL`: result, extent, data, alignment, capacity/reuse invariant, lifecycle, or error behavior is wrong.
- `CRASH`: signal, abnormal termination, timeout, or deadlock prevents use.
- `SETUP ERROR`: build/fixture/oracle/harness failure; excluded from target counts.

For A30, CRASH describes the observed robustness outcome but is explicitly tagged `NON-CONTRACT`; it is not automatically a correctness bug.

## Harness validation plan

After the suite exists, inject controlled wrong observations into 5 representative result streams:

1. A09: force a non-zero alignment remainder.
2. A10: flip one expected zero byte.
3. A14: replace the reused pointer ID.
4. A23: corrupt one preserved prefix byte.
5. A27: remove one matching `munmap` event.

Each mutation must change the case from PASS to FAIL without changing target execution. This is PHASE 7 work, not CHECKPOINT A work.

## Stop condition

Do not create the 30 cases, execute target semantics, or modify source until this PROFILE A CHECKPOINT A plan is approved.
