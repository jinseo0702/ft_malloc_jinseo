# ft_malloc_jinseo audit — CHECKPOINT B

Status: **CHECKPOINT B reached; next repository/profile pairing requires user direction.**

Current direction selected after that checkpoint: **`ft_malloc_jinseo + PROFILE A`**. The allocator campaign has reached [PROFILE A CHECKPOINT B](profile_a/checkpoint_b.md) with 28 PASS, 1 PARTIAL, 1 FAIL, and 0 CRASH. The PROFILE B files below remain preserved as mismatch history.

Requested profile: **PROFILE B — gnu_nm_project**  
Target repository: **ft_malloc_jinseo**

The requested profile and the repository's implemented product do not match. The repository is a user-space allocator shared library, while PROFILE B assumes an executable that reads ELF files and emits `nm`-style symbol records. This mismatch is preserved as evidence; the audit did not silently substitute PROFILE A.

Files at this checkpoint:

- `baseline.md`: repository and toolchain baseline, isolated build result, source-integrity hashes
- `architecture.md`: architecture reconstructed from the current code
- `feature_inventory.md`: actual allocator inventory plus PROFILE B applicability inventory
- `test_plan.md`: proposed PROFILE B oracle, fixtures, semantic comparison, and normalization rules
- `tools/profile_b_applicability.sh`: reproducible PROFILE B target-interface gate
- `raw/`: unmodified gate/build/artifact evidence
- `test_results.md` / `test_results.json`: gate result and zero-execution semantic counts
- `failures.md`: failure-versus-scope classification
- `checkpoint_b.md`: CHECKPOINT B report and possible next architectures

No existing source file was edited. After CHECKPOINT A approval, the applicability gate ran in an isolated copy and returned `PROFILE NOT APPLICABLE`. No semantic fixture was generated and no allocator semantic test was run.

Evidence labels used throughout:

- `VERIFIED FROM CODE`
- `VERIFIED FROM RUNTIME TEST`
- `USER-PROVIDED RATIONALE`
- `INFERENCE`
- `UNKNOWN`
