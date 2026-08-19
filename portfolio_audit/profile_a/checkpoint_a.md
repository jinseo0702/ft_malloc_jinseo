# PROFILE A — CHECKPOINT A

## 1. Architecture

```text
malloc/free/realloc/calloc or direct ft_* API
                    ↓
global-mutex public wrapper
                    ↓
no-lock allocation/reallocation/free implementation
      ┌─────────────┼─────────────┐
      ↓             ↓             ↓
TINY fixed slots  SMALL fixed slots  LARGE direct mmap
      └─────────────┼─────────────┘
                    ↓
inline metadata + payload; bitmap clear or munmap on free
```

Full evidence: `architecture.md`.

## 2. Feature inventory

Implemented code paths exist for malloc, free, realloc, calloc, TINY/SMALL bitmap reuse, LARGE mmap/munmap, a global API mutex, and allocation diagnostics. General invalid-pointer ownership validation is partial. Aligned-allocation-family APIs are not implemented.

No semantic correctness status is claimed before runtime tests. Full table: `feature_inventory.md`.

## 3. Current functional scope

- 4 overridden standard allocation APIs and 4 `ft_*` entry points.
- 3 allocation classes.
- TINY: 100 × 96-byte slots, intended payload 1–80 bytes.
- SMALL: 100 × 512-byte slots, intended payload 81–496 bytes.
- LARGE: above 496 bytes, at most 100 concurrent table entries.
- first-free fixed-slot reuse; no split/coalesce policy.

All values are VERIFIED FROM CODE and describe scope/capacity, not correctness or efficiency.

## 4. Important design characteristics

- segregated fixed slots for small requests and direct mapping for large requests;
- inline metadata immediately before the returned payload;
- lazy fixed-arena creation with retained TINY/SMALL mappings;
- individually released LARGE mappings;
- one coarse global mutex around each public allocation transaction;
- no-lock internal helpers used for realloc/calloc composition;
- hard per-class capacity of 100.

## 5. Baseline test plan

Exactly 30 top-level deterministic cases cover:

- direct API and LD_PRELOAD integration;
- zero, 1-byte, and all zone boundaries;
- alignment and full requested-extent access;
- calloc zeroing/overflow;
- free-null and bitmap reuse;
- per-zone capacity/recovery;
- realloc null/zero/in-zone/cross-zone/failure preservation;
- fixed-arena growth and LARGE mapping balance;
- diagnostics, controlled concurrency, and non-contract robustness.

Tests run in fresh subprocesses against an isolated build. Setup failures are separated from target results. Full case matrix: `test_plan.md`.

## 6. Reference / oracle

- Primary: C allocation API semantics and observable invariants.
- Project-policy oracle: current code's boundaries, capacities, reuse order, and mapping lifecycle.
- System-call oracle: strace 6.19 with mapping sizes derived from the runtime 4096-byte page size.
- Relative diagnostic only: glibc 2.43; not an oracle for pointer identity, capacity, syscall count, memory use, or speed.
- Valgrind is unavailable and is not assumed by the plan.

## 7. Normalization

ASLR addresses, temporary roots, strace PID/timestamps, and thread log order may be normalized only after semantic relationships are evaluated. Nullness, alignment, data, pointer reuse, zone boundaries, counts, mmap/munmap lifecycle, diagnostic sizes/totals, exits, signals, and timeouts are never normalized.

## Approval boundary

Stop at CHECKPOINT A. No PROFILE A runtime semantic case or source change is authorized until the user approves this plan.
