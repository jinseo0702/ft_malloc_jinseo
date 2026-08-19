# Improvement Candidate Selection — PHASE 10

No project source change has been made. This document recommends one candidate and defines the design/acceptance boundary that still needs explicit approval.

## Candidate comparison

| Criterion | A28 — diagnostic snapshot | A30 — ownership validation |
|---|---|---|
| Observed problem | valid allocations omitted; totals wrong in 8/10 repeats | forged pointer created live alias in 10/10 repeats; double free undetected |
| Requirement basis | declared diagnostic feature; concurrent safety intended by user | invalid/double-free defense explicitly identified as an original requirement |
| Correctness impact | diagnostic state/reporting; core allocation suite otherwise passed | allocator ownership can be corrupted, permitting two live handles to one slot |
| Architectural interest | low for counter initialization; medium/high for coherent concurrent snapshot | high: pointer classification, range/alignment checks, live-bit/table invariants, error policy |
| Implementation cost | low for measured defect; medium for concurrent snapshot | medium: refactor free classification without trusting foreign metadata |
| Regression surface | `show_alloc_mem` plus concurrent diagnostic use | free/reuse for all three classes, realloc cleanup, invalid/double free, concurrency |
| Portfolio story | clean and easy Before/After, but narrow if only counter initialization | stronger problem→invariant→architecture→regression narrative |

## Recommendation

**Recommend A30 — ownership-derived free validation as the primary improvement candidate.**

Why:

1. It was an original requirement, according to USER-PROVIDED RATIONALE.
2. Runtime evidence demonstrates state corruption rather than only incorrect reporting.
3. The solution requires a meaningful architectural decision: ownership must be derived from allocator-managed ranges/tables before client-adjacent metadata is trusted.
4. It produces a stronger portfolio case than initializing one local counter.

If the goal were simply the fastest valid-input correctness repair, A28 would be the better first patch. For portfolio engineering value, A30 is stronger.

## Recommended A30 architecture

```text
ft_free(ptr) while global allocator mutex is held
                    ↓
convert ptr to integer address; do not dereference foreign metadata
                    ↓
classify ownership from allocator state
  ├─ TINY arena range + exact payload-slot alignment
  ├─ SMALL arena range + exact payload-slot alignment
  └─ exact LARGE table payload match
                    ↓
verify corresponding bitmap bit / LARGE entry is live
  ├─ valid live allocation → clear bit or munmap
  └─ foreign/already-free → deterministic invalid-free policy
```

### TINY/SMALL validation

- Compare integer addresses against the mapped arena range before any metadata access.
- Require the pointer to equal `arena_base + slot_index × slot_stride + sizeof(METADATA_t)` for an index in 0–99.
- Derive bitmap byte/bit from the validated slot index rather than from caller-adjacent metadata.
- Reject an already-clear bit as invalid/double free.

### LARGE validation

- Scan the bounded LARGE table for an exact match between `ptr` and each tracked mapping's payload address.
- Read mapping length/metadata only after an exact tracked entry is found.
- A missing table match is invalid/double free; it must not call `munmap`.

### Known limit

A raw C pointer cannot distinguish a stale pointer after the same slot has been freed and legitimately reallocated. The proposed validation detects foreign pointers and double free while the slot is not live; generation-safe stale-pointer detection would require a different API/handle model and is outside this allocator's current interface.

## Invalid-free response policy

The remaining design choice is what a rejected free should do:

- **Safe no-op (recommended for the interposed production path):** preserve allocator state and return. Lowest recursion/integration risk, but silent.
- **Minimal diagnostic then return:** emit a fixed message through a low-level, allocation-free output path; better debugging, with some reentrancy/output risk.
- **Fail fast:** terminate on invalid/double free; easiest to detect but disruptive for `LD_PRELOAD` integration and requires a clearly intentional contract.

Recommendation: safe no-op in the normal path, with an optional allocation-free debug diagnostic if the project wants observability.

## Acceptance criteria after approval

These are targets, not claimed improvements:

- A30 forged foreign pointer no longer produces a live alias in any of 10 fresh-process repeats.
- Immediate double free does not change bitmap/table state, unmap unrelated memory, crash, or deadlock.
- Valid TINY, SMALL, and LARGE frees retain their existing reuse/lifecycle behavior.
- Realloc cleanup paths continue to pass.
- The entire 30-case baseline suite is rerun; no previous PASS may regress.
- Before/After is reported only after the new measurements exist.

A28 remains an independent known FAIL unless separately approved; it must not be silently bundled into the A30 change.

## Approval boundary

Source modification is still prohibited. To start PHASE 11, explicitly approve A30 ownership validation and select the rejected-free response policy (`safe no-op`, `diagnostic then return`, or `fail fast`).
