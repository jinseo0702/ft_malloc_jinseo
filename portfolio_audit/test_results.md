# PROFILE B Test Results

## Result summary

| Measure | Result | Classification |
|---|---:|---|
| Unique applicability gate cases | 1 | VERIFIED FROM CODE |
| Gate executions | 2 | VERIFIED FROM RUNTIME TEST |
| Stable gate result across executions | yes | VERIFIED FROM RUNTIME TEST (`cmp` exit 0) |
| Gate result | PROFILE NOT APPLICABLE | VERIFIED FROM CODE / VERIFIED FROM RUNTIME TEST |
| Planned semantic cases | 20 | VERIFIED FROM CODE (`test_plan.md`, B01–B20) |
| Semantic cases executed | 0 | VERIFIED FROM RUNTIME TEST |
| Excluded by applicability gate | 20 | VERIFIED FROM RUNTIME TEST |
| PASS | 0 | VERIFIED FROM RUNTIME TEST |
| PARTIAL | 0 | VERIFIED FROM RUNTIME TEST |
| FAIL | 0 | VERIFIED FROM RUNTIME TEST |
| CRASH | 0 | VERIFIED FROM RUNTIME TEST |
| SETUP ERROR | 0 | VERIFIED FROM RUNTIME TEST |

Zero PASS does not mean the target failed every case. No semantic comparison was possible, so all 20 planned cases were excluded before execution. Zero FAIL/PARTIAL/CRASH likewise must not be presented as correctness evidence.

## PHASE 4 — deterministic suite gate

`portfolio_audit/tools/profile_b_applicability.sh` copied the repository to a fresh `/tmp` directory, built the default root target, inspected the artifact, and searched the production source directories for a production entry point and direct ELF-parser signals.

Observed gate evidence:

```text
classification=PROFILE NOT APPLICABLE
reason=no_production_elf_input_symbol_output_command
isolated_build_exit=0
artifact_present=true
artifact_is_shared_object=true
production_main_matches=0
elf_parser_signal_matches=0
target_command_available=false
```

The gate was executed twice and its normalized `.env` result was byte-identical. The build success is a setup observation, not a semantic PROFILE B PASS. Raw evidence is preserved under `portfolio_audit/raw/`.

## PHASE 5 — oracle comparison

GNU nm 2.46 is installed and was version-recorded. It was used only to inspect the isolated allocator shared library's exported symbols as supporting artifact evidence. It was not compared with target-produced nm output because the target has no ELF-input/symbol-output command.

## PHASE 6 — classification

The gate result is `PROFILE NOT APPLICABLE`, an exclusion classification defined at CHECKPOINT A. It is intentionally outside PASS/PARTIAL/FAIL/CRASH counts.

## PHASE 7 — harness validation

The 5 planned mutation checks were not run. With no target records and no semantic comparator execution, injected symbol mistakes would test a fabricated pipeline rather than this repository.

## PHASE 8 — failure analysis

There are no target FAIL, PARTIAL, or CRASH cases to analyze. The profile/repository mismatch is documented as a scope/applicability issue, not relabeled as an allocator correctness bug.
