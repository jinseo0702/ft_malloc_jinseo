#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import re
import sys


def load_runner(path: Path):
    spec = importlib.util.spec_from_file_location("profile_a_runner", path)
    if spec is None or spec.loader is None:
        raise RuntimeError("could not load suite runner")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def remove_first_target_munmap(text: str) -> str:
    marker = text.find("AUDIT_A27_BEGIN")
    if marker < 0:
        return text
    prefix = text[:marker]
    target_segment = text[marker:]
    target_segment = re.sub(
        r"^munmap\([^\n]*\)\s+=\s*0\s*\n", "", target_segment,
        count=1, flags=re.MULTILINE,
    )
    return prefix + target_segment


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} REPOSITORY_ROOT", file=sys.stderr)
        return 2
    repo_root = Path(sys.argv[1]).resolve()
    profile_root = repo_root / "portfolio_audit/profile_a"
    raw_cases = profile_root / "raw/run_001/cases"
    output_root = profile_root / "raw/run_001/harness_validation_retry"
    if output_root.exists():
        print(f"refusing to overwrite {output_root}", file=sys.stderr)
        return 2
    output_root.mkdir(parents=True)
    runner = load_runner(profile_root / "tools/run_suite.py")
    page_size = 4096
    mutations = []

    plans = [
        ("A09", "stdout", lambda text: text.replace("remainder=0", "remainder=1", 1), "non-zero alignment remainder"),
        ("A10", "stdout", lambda text: text.replace("zero_bytes=221", "zero_bytes=220", 1), "missing calloc zero byte"),
        ("A14", "stdout", lambda text: text.replace("reused=1", "reused=0", 1), "wrong reuse pointer relation"),
        ("A23", "stdout", lambda text: text.replace("prefix=400", "prefix=399", 1), "corrupted realloc prefix evidence"),
        (
            "A27", "trace",
            remove_first_target_munmap,
            "missing munmap event",
        ),
    ]

    for case_id, channel, mutate, description in plans:
        case_root = raw_cases / case_id
        stdout = (case_root / "stdout.raw").read_text(encoding="utf-8", errors="replace")
        stderr = (case_root / "stderr.raw").read_text(encoding="utf-8", errors="replace")
        trace_path = case_root / "strace.raw"
        trace = trace_path.read_text(encoding="utf-8", errors="replace") if trace_path.exists() else ""
        original = runner.evaluate_semantics(case_id, stdout, stderr, trace, page_size)
        mutated_stdout = mutate(stdout) if channel == "stdout" else stdout
        mutated_trace = mutate(trace) if channel == "trace" else trace
        mutated = runner.evaluate_semantics(case_id, mutated_stdout, stderr, mutated_trace, page_size)
        mutated_path = output_root / f"{case_id}.{channel}.mutated.raw"
        mutated_path.write_text(mutated_stdout if channel == "stdout" else mutated_trace, encoding="utf-8")
        detected = original[0] == "PASS" and mutated[0] == "FAIL"
        mutations.append({
            "case_id": case_id,
            "description": description,
            "original_classification": original[0],
            "mutated_classification": mutated[0],
            "mutated_reason": mutated[1],
            "detected": detected,
            "mutated_raw": str(mutated_path),
        })

    summary = {
        "planned": 5,
        "executed": len(mutations),
        "detected": sum(1 for item in mutations if item["detected"]),
        "all_detected": all(item["detected"] for item in mutations),
        "mutations": mutations,
    }
    output_path = profile_root / "harness_validation.json"
    output_path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(summary, sort_keys=True))
    return 0 if summary["all_detected"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
