#!/usr/bin/env python3

import json
from pathlib import Path
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} REPOSITORY_ROOT", file=sys.stderr)
        return 2
    repo_root = Path(sys.argv[1]).resolve()
    profile = repo_root / "portfolio_audit/profile_a"
    results_path = profile / "test_results.json"
    results = json.loads(results_path.read_text(encoding="utf-8"))
    repeat_a28 = json.loads((profile / "raw/run_001/repeats/A28/summary.json").read_text(encoding="utf-8"))
    repeat_a30 = json.loads((profile / "raw/run_001/repeats/A30_FOREIGN/summary.json").read_text(encoding="utf-8"))
    validation = json.loads((profile / "harness_validation.json").read_text(encoding="utf-8"))
    results["repeat_verification"] = {
        "A28": {
            "executions": repeat_a28["executions"],
            "correct_snapshots": sum(1 for item in repeat_a28["records"] if item["totals"] == [710, 610]),
            "mismatched_snapshots": sum(1 for item in repeat_a28["records"] if item["totals"] != [710, 610]),
            "observed_totals": [item["totals"] for item in repeat_a28["records"]],
        },
        "A30_FOREIGN": {
            "executions": repeat_a30["executions"],
            "live_alias_observed": sum(1 for item in repeat_a30["records"] if item["foreign_alias"]),
        },
    }
    results["harness_validation"] = {
        "planned": validation["planned"],
        "executed": validation["executed"],
        "detected": validation["detected"],
        "all_detected": validation["all_detected"],
        "initial_attempt_detected": 4,
        "initial_attempt_note": "first A27 mutation removed a startup munmap outside the marked target region; mutation builder was corrected and rerun",
    }
    results_path.write_text(json.dumps(results, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({
        "counts": results["counts"],
        "repeat_verification": results["repeat_verification"],
        "harness_validation": results["harness_validation"],
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
