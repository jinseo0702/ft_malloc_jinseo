# Design Rationale — PHASE 9

This document separates code/runtime evidence, author-provided rationale, and analyst inference. User answers were received after PROFILE A CHECKPOINT B.

## Verified facts

### VERIFIED FROM CODE

- Public `ft_malloc`, `ft_free`, `ft_realloc`, and `ft_calloc2` operations are serialized by one global mutex.
- `show_alloc_mem` traverses the same global allocation state without acquiring that mutex.
- TINY and SMALL use fixed arrays of 100 slots; LARGE admission is bounded by `large_cnt < 100`.
- TINY/SMALL frees clear a bitmap bit but retain their arenas. LARGE free clears its table entry and calls `munmap`.
- `free_impl` derives metadata from every non-null pointer before proving that the pointer belongs to a live allocation.

### VERIFIED FROM RUNTIME TEST

- A28 produced incorrect `show_alloc_mem` snapshots in 8/10 fresh-process repeats.
- A30's forged foreign pointer created a duplicate live-slot alias in 10/10 fresh-process repeats.
- Boundary, alignment, capacity/recovery, realloc, fixed-arena mapping, LARGE mapping balance, and a controlled four-thread allocation workload passed the baseline suite.

## User-provided rationale

### USER-PROVIDED RATIONALE

- `show_alloc_mem` is intended to be safe around concurrent allocation/free activity, while the author expects deadlock or data-race risk may exist in the current design.
- Invalid-free and double-free defense was an original project requirement.
- The fixed capacity of 100 was chosen because the goal was to understand allocation mechanisms, not build a general-purpose allocator.
- LARGE allocations are individually acquired and released because, above that size, obtaining the required pages and returning them was prioritized over subdividing them.
- The author has not yet selected A28 or A30 as the improvement priority and requested additional explanation.

## Inference

### INFERENCE

- The 100-entry limit is a deliberate educational scope boundary, not an accidental scalability claim.
- Retaining fixed arenas and directly unmapping LARGE allocations expresses two different lifecycle policies: reusable pooled storage versus individually owned page mappings.
- Because invalid/double-free defense was an original requirement, A30 is not merely an optional hardening idea in the portfolio narrative; it is evidence of an unmet project-specific invariant.
- A28 has two layers: the measured single-thread counter-initialization defect and an unmeasured concurrent-snapshot question. Fixing only the counter would not by itself prove the user's intended concurrent safety.

## Unknown

### UNKNOWN

- Whether invalid/double free should be ignored, reported and continued, or fail fast.
- Whether concurrent `show_alloc_mem` requires only no crash/data race or a fully atomic point-in-time snapshot.
- Which rejected allocator architectures were considered during the original implementation.
- What the author considered the hardest original design decision.
- What the author would change if rebuilding the allocator from scratch beyond the two measured candidates.

## Classification update after rationale

The baseline result remains reproducible as `A30 PARTIAL (NON-CONTRACT)` under the generic C API oracle. After incorporating the USER-PROVIDED RATIONALE that invalid/double-free defense was required, the same runtime evidence also represents a project-requirement failure. Both views are retained rather than rewriting the original baseline count.
