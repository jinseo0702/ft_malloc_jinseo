# Baseline Test Plan — requested PROFILE B

At CHECKPOINT A, no test in this plan had been created or executed. After approval, the applicability gate was created and executed; it returned `PROFILE NOT APPLICABLE`, so B01–B20 remain uncreated and unexecuted. See `test_results.md`.

## Applicability gate

Before generating a comparison matrix, the harness will require a target command satisfying all of these conditions:

1. accepts a path to an ELF file or static archive;
2. emits symbol records containing at least a symbol name and classification;
3. returns distinguishable success/error status;
4. can be invoked deterministically without modifying the fixture.

**Current result from code/build inspection:** the repository has no such target command. This is `NOT IMPLEMENTED` relative to PROFILE B, not a runtime allocator failure. GNU nm output produced by inspecting `libft_malloc_*.so` must not be mislabeled as target output.

If the user approves continuing with PROFILE B unchanged and no target interface is added, the gate will stop semantic cases as `PROFILE NOT APPLICABLE`; it will not manufacture PASS/FAIL counts. If the intended profile was PROFILE A, that is a separate user-authorized plan change.

## Oracle

Primary oracle:

- GNU nm `2.46`, the version installed in the recorded baseline environment.
- Execute as `LC_ALL=C nm <fixture>` with an explicitly recorded option set.

Secondary diagnostic tools, not automatic PASS/FAIL oracles:

- `readelf -h -S -s -W` to inspect ELF headers, sections, and symbol records when a difference needs explanation.
- `ar t` for archive member topology.
- the fixture source/assembly and its deterministic build recipe for intended bindings, definitions, and section placement.

GNU nm is authoritative for requested output semantics. Secondary tools are used only to localize disagreements or to identify an oracle/fixture setup mistake.

## Proposed deterministic fixture set

After the applicability gate passes, start with these 20 cases:

| ID | Fixture / behavior | Primary semantic focus |
|---|---|---|
| B01 | unstripped relocatable C object | baseline names, values, types, ordering |
| B02 | non-PIE executable | executable symbol table |
| B03 | shared object | dynamic/shared-object symbols |
| B04 | two-member static archive | member headings and per-member ordering |
| B05 | stripped executable | no-symbol/error behavior |
| B06 | global defined text/data/bss symbols | `T/D/B`-class semantics |
| B07 | local defined text/data/bss symbols | lowercase/local semantics and inclusion |
| B08 | undefined external symbol | undefined value field and `U` classification |
| B09 | defined weak symbol | weak-defined classification |
| B10 | undefined weak symbol | weak-undefined classification |
| B11 | read-only data symbol | read-only section classification |
| B12 | absolute symbol from a minimal assembly fixture | absolute classification |
| B13 | common symbol compiled with an explicit compatible compiler flag | common classification |
| B14 | duplicate symbol names in distinct archive members | member identity and inclusion |
| B15 | empty valid relocatable object | empty-output behavior |
| B16 | truncated ELF header | malformed-input status and stderr |
| B17 | valid ELF header with corrupt section bounds | bounds-check/error behavior |
| B18 | plain-text input | unrecognized-format behavior |
| B19 | missing path | filesystem error behavior |
| B20 | directory path | non-regular-input error behavior |

No option-specific target cases are planned because **0 target options are implemented**. Options will be added only if later source evidence establishes their existence.

Each case stores the exact fixture hash, build command, target stdout/stderr/exit status, and GNU nm stdout/stderr/exit status under `portfolio_audit/raw/`.

## Comparison model

Successful symbol output will be parsed, without discarding order, into records:

```text
container/member | symbol name | symbol value or absent | symbol type | ordinal
```

Compare:

- exact symbol name;
- numeric symbol value, including whether the value is omitted for undefined symbols;
- symbol type character and case;
- inclusion/exclusion (missing and extra records);
- record ordering within each input/member;
- archive member association;
- exit status and error category;
- crash, signal, or timeout separately from ordinary error output.

A string merely appearing in output is not a PASS. Setup/build failure is recorded separately and never charged to the target.

## Normalization

Normalized:

- force `LC_ALL=C` for both implementations;
- use the exact same fixture bytes for target and oracle;
- parse legal nm whitespace into fields rather than comparing column padding;
- normalize only the executable-name prefix in stderr (for example `nm:` versus the target command name) while preserving the referenced path and complete diagnostic body;
- replace the harness-owned temporary fixture-root prefix with `<FIXTURE_ROOT>` in saved display copies; raw output remains untouched;
- represent an oracle-omitted undefined-symbol value as an explicit `ABSENT` field, not as numeric zero.

Not normalized:

- missing or extra symbols;
- symbol names;
- symbol numeric values;
- symbol type or letter case;
- ordering;
- archive member identity or headings;
- inclusion/exclusion decisions;
- exit status, signal, crash, or timeout;
- malformed-input acceptance versus rejection;
- substantive diagnostic category or referenced input path.

Because both programs receive the same deterministic fixture, address/value differences are not presumed harmless. Any difference must be explained from the fixture and ELF semantics before a normalization rule can be added.

## Planned classification

- `PASS`: all core semantic fields and error behavior match the oracle for the case.
- `PARTIAL`: core behavior is present, but meaningful symbol information or a semantic sub-behavior is absent.
- `FAIL`: symbol identity/value/type/inclusion/order or error behavior is wrong.
- `CRASH`: abnormal termination, signal, timeout, or deadlock prevents use.
- `SETUP ERROR`: fixture/oracle/harness creation failed; excluded from target result counts.
- `PROFILE NOT APPLICABLE`: the initial target-interface gate is not satisfied; excluded from PASS/PARTIAL/FAIL/CRASH counts.

## Harness validation after approval

Once a target interface exists, validate 5 representative cases by injecting a controlled wrong result:

1. B01: remove one symbol → must detect missing symbol.
2. B06: alter one type character → must detect type mismatch.
3. B08: change `ABSENT` undefined value to `0` → must detect value-presence mismatch.
4. B04: swap archive member association → must detect member mismatch.
5. B16: force success on malformed input → must detect exit/error mismatch.

This validation is planned only; executing it belongs to PHASE 7, after the approved PHASE 4–6 suite exists.

## Stop condition

This document completes the CHECKPOINT A test-plan requirement. Do not create fixtures, harness code, comparison results, failures, or source changes until the user approves the next phase or corrects the requested profile.
