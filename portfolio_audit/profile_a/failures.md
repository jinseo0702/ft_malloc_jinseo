# PROFILE A Failure Analysis

## A28 — allocation diagnostics omit live entries

**Classification:** FAIL — correctness bug in `show_alloc_mem` diagnostics, not in the core allocation result path.

**Expected**

With live allocations of 10 (TINY), 100 (SMALL), and 600 (LARGE) bytes, the first snapshot must include all three and total 710. After freeing the SMALL allocation, it must include TINY and LARGE and total 610.

**Actual**

The baseline execution omitted the 10-byte TINY entry in the first snapshot and printed 700, then printed the correct second total 610. Ten fresh-process repeats produced only 2 fully correct pairs and 8 mismatches:

```text
[710,600] × 3
[700,610] × 5
[710,610] × 2
```

**Relevant code path**

`src/ft_malloc.c:400-445`, especially line 403 and the TINY loop at lines 404-418.

**Root cause**

At line 403, `unsigned char find, cnt, cnt2 = 0x00;` initializes only `cnt2`. `cnt` is indeterminate but controls `if (cnt >= 100)` and is incremented during the TINY scan. Stack residue can therefore skip some or all TINY entries. Each function call receives a new indeterminate value, explaining why either snapshot may omit the 10-byte allocation.

**Evidence**

- baseline raw: `raw/run_001/cases/A28/stdout.raw`
- repeat summary: `raw/run_001/repeats/A28/summary.json`
- source evidence: `src/ft_malloc.c:403-418`

**Severity**

Medium for diagnostics: live allocations and total bytes can be underreported. The suite did not observe a core malloc/free/realloc/calloc failure from this defect.

**Possible fix directions — not applied**

1. Minimal deterministic fix: explicitly initialize both counters.
2. Locked traversal: initialize counters and protect the complete `show_alloc_mem` traversal with the allocator mutex.
3. Snapshot architecture: copy scalar allocation records while locked, unlock, then format/print the immutable snapshot.

## A30 — foreign metadata can corrupt live-slot ownership

**Baseline classification:** PARTIAL, tagged `NON-CONTRACT` — robustness limitation under the generic C allocation contract.

**Post-CHECKPOINT-B requirement classification:** USER-PROVIDED RATIONALE states invalid/double-free defense was an original requirement. Relative to that project-specific requirement, the same observation is an unmet requirement and a valid improvement candidate. The baseline count is preserved rather than rewritten retroactively.

**Expected for a defensive allocator**

A pointer that is not an exact live allocation payload should not clear a real allocation's bitmap state. A previously freed pointer should be detected or ignored without altering unrelated live ownership.

**Actual**

A crafted foreign pointer with plausible preceding metadata cleared TINY slot 0 while the real slot was still live. The next allocation returned the same address, creating two client-visible live aliases. This occurred in 10/10 fresh subprocess repeats. A separate double-free subprocess completed without detection.

**Relevant code path**

`src/ft_malloc.c:727-765`, especially metadata recovery at lines 733-739 and bitmap mutation at lines 740-748.

**Root cause**

`free_impl` subtracts one `METADATA_t` from every non-null input and trusts the recovered `size`, bitmap index, and position before establishing that the pointer belongs to a live TINY/SMALL slot or a tracked LARGE mapping. A forged metadata tuple can therefore mutate allocator state.

**Evidence**

- baseline raw: `raw/run_001/cases/A30/foreign.stdout.raw`
- repeat summary: `raw/run_001/repeats/A30_FOREIGN/summary.json`
- source evidence: `src/ft_malloc.c:733-759`

**Severity**

High relative to the user-confirmed project requirement because allocator ownership can be corrupted. It remains outside the portable C caller contract, so the portfolio description must state both boundaries.

**Possible fix directions — not applied**

1. Derive ownership from arena ranges, slot alignment, bitmap state, and exact LARGE table matches before reading client-adjacent metadata.
2. Reject/no-op a TINY/SMALL free when the derived slot bit is already clear; require exact live LARGE table membership before unmapping.
3. In a debug mode, add allocation state/canary data and emit a deterministic diagnostic for invalid or double free.

## Classification boundary

- A28 is a reproducible correctness bug in a declared project diagnostic feature.
- A30 is a reproducible robustness limitation outside normal C caller obligations.
- Fixed TINY/SMALL arena retention and the 100-allocation limits are implemented scope/policy, not failures by themselves.
- No performance issue was measured.
