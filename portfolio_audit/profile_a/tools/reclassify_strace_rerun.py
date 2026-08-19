#!/usr/bin/env python3

from __future__ import annotations

import collections
import importlib.util
import json
from pathlib import Path
import sys


def load_runner(path: Path):
    spec = importlib.util.spec_from_file_location("profile_a_runner", path)
    if spec is None or spec.loader is None:
        raise RuntimeError("could not load suite runner")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} REPOSITORY_ROOT", file=sys.stderr)
        return 2
    repo_root = Path(sys.argv[1]).resolve()
    profile_root = repo_root / "portfolio_audit/profile_a"
    summary_path = profile_root / "test_results.json"
    raw_root = profile_root / "raw/run_001"
    runner = load_runner(profile_root / "tools/run_suite.py")
    summary = json.loads(summary_path.read_text(encoding="utf-8"))

    for case_id in ("A26", "A27"):
        case_root = raw_root / "cases" / case_id
        stdout = (case_root / "stdout.raw").read_text(encoding="utf-8", errors="replace")
        stderr = (case_root / "stderr.raw").read_text(encoding="utf-8", errors="replace")
        trace = (case_root / "strace.raw").read_text(encoding="utf-8", errors="replace")
        classification, reason, facts = runner.evaluate_semantics(
            case_id, stdout, stderr, trace, summary["page_size"]
        )
        if classification != "PASS":
            raise RuntimeError(f"{case_id} escalated strace rerun did not pass: {reason}")
        result = next(item for item in summary["results"] if item["id"] == case_id)
        result["initial_execution"] = result["execution"]
        result["classification"] = classification
        result["reason"] = reason
        result["facts"] = facts
        result["execution"] = {
            "return_code": 0,
            "timed_out": False,
            "rerun": True,
            "rerun_reason": "initial sandbox denied ptrace; repeated with approved strace permission",
            "raw_directory": str(case_root),
        }
        (case_root / "result.json").write_text(
            json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )

    counts = collections.Counter(item["classification"] for item in summary["results"])
    summary["counts"] = {
        name: counts.get(name, 0) for name in ("PASS", "PARTIAL", "FAIL", "CRASH")
    }
    summary["environment_retries"] = {
        "count": 2,
        "cases": ["A26", "A27"],
        "reason": "sandbox ptrace denial",
        "resolved": True,
    }
    summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(summary["counts"], sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
