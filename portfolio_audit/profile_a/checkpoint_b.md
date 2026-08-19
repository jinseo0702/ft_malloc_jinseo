# PROFILE A — CHECKPOINT B

## 1. Baseline result

30/30 planned top-level cases executed against an isolated build:

```text
PASS     28
PARTIAL   1
FAIL      1
CRASH     0
```

Setup error: 0. Source hashes remained unchanged. Harness mutation validation detected 5/5 injected wrong results after correcting one mutation-builder targeting error.

## 2. Most important PASS cases

- A05–A08: all exact class boundaries (80/81/496/497) returned writable extents.
- A09: all six representative allocations met measured 16-byte `max_align_t` alignment.
- A16–A18: each class matched its 100-allocation capacity and recovered after a free.
- A19–A25: realloc null, zero, same-zone, cross-zone, shrink, and forced-failure invariants passed.
- A26–A27: fixed arenas mapped once and all 10 LARGE mappings were paired with unmaps.
- A29: four threads completed 2000 controlled operations with no detected integrity failure.

## 3. Most important non-PASS cases

Only two non-PASS cases exist; no additional failures are fabricated to reach 3–5.

1. **A28 FAIL:** `show_alloc_mem` underreported live allocations. Only 2/10 repeated executions produced both correct totals; root cause is an uninitialized TINY loop counter.
2. **A30 PARTIAL / NON-CONTRACT:** crafted foreign metadata cleared a live bitmap slot and created an alias in 10/10 repeats; double free was not detected.

## 4. Correctness bug versus scope limitation

- **Correctness bug:** A28, because a declared diagnostic function reports incorrect live state under valid use.
- **Robustness limitation:** A30, because invalid/double-free input is outside the C contract but can deterministically corrupt allocator ownership.
- **Scope/policy, not a bug:** fixed 100-slot limits, first-free policy, and retaining TINY/SMALL arenas after free.
- **UNKNOWN:** whether invalid-free detection and concurrent `show_alloc_mem` are intended project promises; code alone cannot supply the author's rationale.

## 5. Highest-value portfolio failure candidate

A28 is the safest correctness case: it is valid-input behavior, reproduced across fresh processes, localized to one code path, and supports a clean Before/After regression story. Its implementation fix is small, so its architectural depth is limited.

A30 has greater architectural interest because a fix requires ownership validation derived from arena/table state, but it should only become the primary engineering case if defensive invalid-free handling was an intended requirement.

## 6. Possible solution architectures

### Option 1 — minimal diagnostic correction

Initialize every traversal counter and add focused A28 regression coverage. Lowest implementation cost; fixes the demonstrated single-thread diagnostic failure.

### Option 2 — locked immutable diagnostic snapshot

Under the allocator mutex, derive and copy live `{zone,start,end,client_size}` records and total into temporary audit state; release the lock before formatting. Addresses the observed counter defect and defines a coherent concurrent snapshot.

### Option 3 — ownership-derived free validation

Classify a pointer from arena ranges/slot alignment or exact LARGE table membership, then consult live bitmap/table state before changing it. Optionally emit debug diagnostics for foreign or repeated frees. Higher implementation cost and only justified if defensive robustness is part of the intended contract.

## Questions for design rationale

1. Was `show_alloc_mem` intended to be safe when another thread allocates or frees concurrently?
2. Should invalid and double free be defensively detected, or was valid C caller behavior the explicit boundary?
3. Why did you choose exactly 100 slots/mappings per class?
4. Why are TINY/SMALL arenas retained after becoming empty while LARGE mappings are immediately unmapped?
5. For the improvement cycle, should the portfolio prioritize A28's standards-safe diagnostic correctness story or A30's more architectural robustness story?

## Stop condition

Stop at CHECKPOINT B. No project source fix has been applied. PHASE 9–11 requires user rationale and explicit approval of one improvement architecture.
