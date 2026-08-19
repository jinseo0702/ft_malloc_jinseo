#!/usr/bin/env python3

from __future__ import annotations

import collections
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import time
from typing import Any


CASE_DESCRIPTIONS = {
    "A01": "direct ft_malloc/write/free smoke",
    "A02": "LD_PRELOAD malloc/free smoke",
    "A03": "malloc zero-size semantics",
    "A04": "1-byte TINY extent",
    "A05": "80-byte TINY boundary",
    "A06": "81-byte SMALL boundary",
    "A07": "496-byte SMALL boundary",
    "A08": "497-byte LARGE boundary",
    "A09": "max_align_t alignment across classes",
    "A10": "calloc zero initialization",
    "A11": "calloc multiplication overflow",
    "A12": "calloc zero operands",
    "A13": "free NULL",
    "A14": "TINY first-free reuse",
    "A15": "SMALL first-free reuse",
    "A16": "TINY capacity and recovery",
    "A17": "SMALL capacity and recovery",
    "A18": "LARGE capacity and recovery",
    "A19": "realloc NULL",
    "A20": "realloc zero and slot reuse",
    "A21": "TINY in-zone realloc",
    "A22": "TINY-to-SMALL realloc",
    "A23": "SMALL-to-LARGE realloc",
    "A24": "LARGE shrink realloc",
    "A25": "realloc failure preserves original",
    "A26": "fixed-arena mapping growth",
    "A27": "LARGE mmap/munmap balance",
    "A28": "show_alloc_mem accounting",
    "A29": "controlled multi-thread integrity",
    "A30": "invalid/double-free non-contract robustness",
}


def write_text(path: Path, value: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value, encoding="utf-8")


def write_json(path: Path, value: Any) -> None:
    write_text(path, json.dumps(value, indent=2, sort_keys=True) + "\n")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def command_text(command: list[str]) -> str:
    return " ".join(json.dumps(item) for item in command)


def run_logged(command: list[str], cwd: Path, stdout_path: Path,
               stderr_path: Path, env: dict[str, str] | None = None,
               timeout: float = 30.0) -> dict[str, Any]:
    started = time.monotonic()
    timed_out = False
    try:
        completed = subprocess.run(
            command,
            cwd=cwd,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
            check=False,
        )
        return_code = completed.returncode
        stdout = completed.stdout
        stderr = completed.stderr
    except subprocess.TimeoutExpired as error:
        timed_out = True
        return_code = None
        stdout = error.stdout or ""
        stderr = error.stderr or ""
        if isinstance(stdout, bytes):
            stdout = stdout.decode("utf-8", errors="replace")
        if isinstance(stderr, bytes):
            stderr = stderr.decode("utf-8", errors="replace")
    duration_ms = int((time.monotonic() - started) * 1000)
    write_text(stdout_path, stdout)
    write_text(stderr_path, stderr)
    return {
        "command": command,
        "command_text": command_text(command),
        "cwd": str(cwd),
        "duration_ms": duration_ms,
        "return_code": return_code,
        "timed_out": timed_out,
    }


def base_classification(meta: dict[str, Any]) -> tuple[str, str]:
    if meta["timed_out"]:
        return "CRASH", "timeout"
    return_code = meta["return_code"]
    if return_code is not None and return_code < 0:
        return "CRASH", f"signal {-return_code}"
    if return_code == 0:
        return "PASS", "process assertions passed"
    return "FAIL", f"process exited {return_code}"


def extract_segment(text: str, begin: str, end: str) -> str:
    begin_index = text.find(begin)
    if begin_index < 0:
        return ""
    end_index = text.find(end, begin_index + len(begin))
    if end_index < 0:
        return ""
    return text[begin_index + len(begin):end_index]


def evaluate_semantics(case_id: str, stdout: str, stderr: str,
                       trace: str = "", page_size: int = 4096) -> tuple[str, str, dict[str, Any]]:
    facts: dict[str, Any] = {}
    if case_id == "A09":
        remainders = [int(value) for value in re.findall(r"remainder=(\d+)", stdout)]
        facts["remainders"] = remainders
        if len(remainders) != 6 or any(value != 0 for value in remainders):
            return "FAIL", "alignment remainder mismatch", facts
    elif case_id == "A10":
        match = re.search(r"zero_bytes=(\d+)", stdout)
        facts["zero_bytes"] = int(match.group(1)) if match else None
        if facts["zero_bytes"] != 221 or "writable=1" not in stdout:
            return "FAIL", "calloc zero/writable evidence mismatch", facts
    elif case_id == "A14":
        facts["reused"] = "reused=1" in stdout
        if not facts["reused"]:
            return "FAIL", "TINY first-free pointer was not reused", facts
    elif case_id == "A23":
        facts["prefix"] = 400 if "prefix=400" in stdout else None
        facts["old_slot_reused"] = "old_slot_reused=1" in stdout
        if facts["prefix"] != 400 or not facts["old_slot_reused"]:
            return "FAIL", "SMALL-to-LARGE preservation/reuse mismatch", facts
    elif case_id == "A26":
        segment = extract_segment(trace, "AUDIT_A26_BEGIN", "AUDIT_A26_END")
        mmap_lengths = [int(value) for value in re.findall(r"mmap\(NULL, (\d+),", segment)]
        tiny_length = ((96 * 100 + page_size - 1) // page_size) * page_size
        small_length = ((512 * 100 + page_size - 1) // page_size) * page_size
        facts.update({
            "mmap_lengths": mmap_lengths,
            "tiny_arena_length": tiny_length,
            "small_arena_length": small_length,
            "tiny_arena_mmaps": mmap_lengths.count(tiny_length),
            "small_arena_mmaps": mmap_lengths.count(small_length),
        })
        if facts["tiny_arena_mmaps"] != 1 or facts["small_arena_mmaps"] != 1:
            return "FAIL", "fixed arenas were not mapped exactly once", facts
    elif case_id == "A27":
        segment = extract_segment(trace, "AUDIT_A27_BEGIN", "AUDIT_A27_END")
        mappings = re.findall(r"mmap\(NULL, (\d+),[^\n]*\) = (0x[0-9a-f]+)", segment)
        unmappings = re.findall(r"munmap\((0x[0-9a-f]+), (\d+)\)\s+= 0", segment)
        mapped = collections.Counter((address, int(length)) for length, address in mappings)
        unmapped = collections.Counter((address, int(length)) for address, length in unmappings)
        facts.update({
            "successful_mmaps": len(mappings),
            "successful_munmaps": len(unmappings),
            "pairs_match": mapped == unmapped,
        })
        if len(mappings) != 10 or len(unmappings) != 10 or mapped != unmapped:
            return "FAIL", "LARGE mmap/munmap lifecycle mismatch", facts
    elif case_id == "A28":
        first = extract_segment(stdout, "AUDIT_SHOW_1_BEGIN", "AUDIT_SHOW_1_END")
        second = extract_segment(stdout, "AUDIT_SHOW_2_BEGIN", "AUDIT_SHOW_2_END")
        first_totals = [int(value) for value in re.findall(r"Total size: (\d+) byte", first)]
        second_totals = [int(value) for value in re.findall(r"Total size: (\d+) byte", second)]
        facts.update({"first_totals": first_totals, "second_totals": second_totals})
        first_ok = (first_totals == [710] and "10 byte tiny" in first
                    and "100 byte small" in first and "600 byte Large" in first)
        second_ok = (second_totals == [610] and "10 byte tiny" in second
                     and "100 byte small" not in second and "600 byte Large" in second)
        facts.update({"first_snapshot_ok": first_ok, "second_snapshot_ok": second_ok})
        if not first_ok or not second_ok:
            return "FAIL", "show_alloc_mem membership/total mismatch", facts
    elif case_id == "A30":
        alias_created = "foreign_free_created_live_alias=1" in stdout
        double_completed = "double_free_completed=1" in stdout
        facts.update({
            "foreign_free_created_live_alias": alias_created,
            "double_free_completed": double_completed,
            "non_contract": True,
        })
        if alias_created:
            return "PARTIAL", "NON-CONTRACT invalid free can corrupt live-slot ownership", facts
        if not double_completed:
            return "FAIL", "robustness subprocess evidence missing", facts
    return "PASS", "semantic assertions passed", facts


def build_suite(repo_root: Path, profile_root: Path, raw_root: Path,
                copy_root: Path) -> dict[str, Any]:
    bin_root = profile_root / "bin"
    bin_root.mkdir(parents=True, exist_ok=True)
    build_root = raw_root / "build"
    build_root.mkdir(parents=True, exist_ok=True)
    build_steps: list[dict[str, Any]] = []

    commands = [
        ["make", "-C", str(copy_root / "libft"), "EXTRA_CFLAG=-fPIC"],
        ["make", "-C", str(copy_root / "printf"), "EXTRA_CFLAG=-fPIC"],
        ["make", "-C", str(copy_root)],
        [
            "gcc", "-std=gnu11", "-g", "-pthread", "-Wall", "-Wextra", "-Werror", "-fPIC",
            "-I", str(copy_root / "include"), "-I", str(copy_root / "printf"),
            "-I", str(copy_root / "libft"), "-c", str(copy_root / "src/ft_malloc.c"),
            "-o", str(bin_root / "ft_malloc_direct.o"),
        ],
        [
            "gcc", "-std=gnu11", "-g", "-pthread", "-Wall", "-Wextra", "-Werror",
            "-I", str(copy_root / "include"), "-I", str(copy_root / "printf"),
            "-I", str(copy_root / "libft"), str(profile_root / "tests/direct_cases.c"),
            str(bin_root / "ft_malloc_direct.o"), str(copy_root / "libft/libft.a"),
            str(copy_root / "printf/libftprintf.a"), "-o", str(bin_root / "direct_cases"),
        ],
        [
            "gcc", "-std=gnu11", "-O0", "-g", "-Wall", "-Wextra", "-Werror",
            str(profile_root / "tests/preload_smoke.c"), "-o", str(bin_root / "preload_smoke"),
        ],
    ]
    for index, command in enumerate(commands, start=1):
        step = run_logged(
            command, repo_root,
            build_root / f"step_{index:02d}.stdout",
            build_root / f"step_{index:02d}.stderr",
            timeout=60.0,
        )
        build_steps.append(step)
        if step["return_code"] != 0 or step["timed_out"]:
            write_json(build_root / "steps.json", build_steps)
            raise RuntimeError(f"build step {index} failed")

    host_type = f"{platform.machine()}_{platform.system()}"
    source_so = copy_root / f"libft_malloc_{host_type}.so"
    target_so = bin_root / source_so.name
    shutil.copy2(source_so, target_so)
    write_json(build_root / "steps.json", build_steps)
    return {
        "direct_binary": bin_root / "direct_cases",
        "preload_binary": bin_root / "preload_smoke",
        "shared_library": target_so,
        "build_steps": build_steps,
    }


def run_case(case_id: str, binaries: dict[str, Any], repo_root: Path,
             raw_root: Path, page_size: int) -> dict[str, Any]:
    case_root = raw_root / "cases" / case_id
    case_root.mkdir(parents=True, exist_ok=True)
    direct_binary = str(binaries["direct_binary"])
    trace_path = case_root / "strace.raw"
    trace_text = ""

    if case_id == "A02":
        command = [str(binaries["preload_binary"])]
        environment = os.environ.copy()
        environment["LD_PRELOAD"] = str(binaries["shared_library"])
        meta = run_logged(command, repo_root, case_root / "stdout.raw",
                          case_root / "stderr.raw", environment, timeout=5.0)
    elif case_id in {"A26", "A27"}:
        command = [
            "strace", "-qq", "-s", "256", "-e", "trace=mmap,munmap,write",
            "-o", str(trace_path), direct_binary, case_id,
        ]
        meta = run_logged(command, repo_root, case_root / "stdout.raw",
                          case_root / "stderr.raw", timeout=15.0)
        trace_text = trace_path.read_text(encoding="utf-8", errors="replace") if trace_path.exists() else ""
    elif case_id == "A30":
        metas = []
        combined_stdout = ""
        combined_stderr = ""
        for label, subcase in (("foreign", "A30_FOREIGN"), ("double", "A30_DOUBLE")):
            submeta = run_logged(
                [direct_binary, subcase], repo_root,
                case_root / f"{label}.stdout.raw", case_root / f"{label}.stderr.raw",
                timeout=5.0,
            )
            metas.append(submeta)
            combined_stdout += (case_root / f"{label}.stdout.raw").read_text(encoding="utf-8")
            combined_stderr += (case_root / f"{label}.stderr.raw").read_text(encoding="utf-8")
        write_text(case_root / "stdout.raw", combined_stdout)
        write_text(case_root / "stderr.raw", combined_stderr)
        timed_out = any(item["timed_out"] for item in metas)
        negative_codes = [item["return_code"] for item in metas
                          if item["return_code"] is not None and item["return_code"] < 0]
        nonzero_codes = [item["return_code"] for item in metas if item["return_code"] not in (0, None)]
        meta = {
            "command": [item["command"] for item in metas],
            "command_text": " ; ".join(item["command_text"] for item in metas),
            "cwd": str(repo_root),
            "duration_ms": sum(item["duration_ms"] for item in metas),
            "return_code": negative_codes[0] if negative_codes else (nonzero_codes[0] if nonzero_codes else 0),
            "timed_out": timed_out,
            "subprocesses": metas,
        }
    else:
        command = [direct_binary, case_id]
        meta = run_logged(command, repo_root, case_root / "stdout.raw",
                          case_root / "stderr.raw", timeout=15.0 if case_id == "A29" else 5.0)

    stdout = (case_root / "stdout.raw").read_text(encoding="utf-8", errors="replace")
    stderr = (case_root / "stderr.raw").read_text(encoding="utf-8", errors="replace")
    classification, reason = base_classification(meta)
    facts: dict[str, Any] = {}
    if classification == "PASS":
        classification, reason, facts = evaluate_semantics(
            case_id, stdout, stderr, trace_text, page_size
        )
    result = {
        "id": case_id,
        "description": CASE_DESCRIPTIONS[case_id],
        "classification": classification,
        "reason": reason,
        "facts": facts,
        "execution": meta,
        "raw_directory": str(case_root),
    }
    write_json(case_root / "result.json", result)
    return result


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} REPOSITORY_ROOT", file=sys.stderr)
        return 2
    repo_root = Path(sys.argv[1]).resolve()
    profile_root = repo_root / "portfolio_audit/profile_a"
    raw_root = profile_root / "raw/run_001"
    if raw_root.exists():
        print(f"refusing to overwrite existing run: {raw_root}", file=sys.stderr)
        return 2
    raw_root.mkdir(parents=True)
    temp_root = Path(tempfile.mkdtemp(prefix="ft_malloc_profile_a."))
    copy_root = temp_root / "repo"
    try:
        shutil.copytree(
            repo_root,
            copy_root,
            ignore=shutil.ignore_patterns(".git", "portfolio_audit"),
        )
        source_paths = [
            repo_root / "Makefile",
            repo_root / "README.md",
            repo_root / "include/ft_malloc.h",
            repo_root / "src/ft_malloc.c",
            repo_root / "src/ft_malloc_ver.0.1.c",
            repo_root / "src/ft_override.c",
        ]
        manifest = {str(path.relative_to(repo_root)): sha256(path) for path in source_paths}
        write_json(raw_root / "source_hashes.before.json", manifest)
        page_size = int(os.sysconf("SC_PAGE_SIZE"))
        binaries = build_suite(repo_root, profile_root, raw_root, copy_root)
        results = [
            run_case(f"A{index:02d}", binaries, repo_root, raw_root, page_size)
            for index in range(1, 31)
        ]
        after_manifest = {str(path.relative_to(repo_root)): sha256(path) for path in source_paths}
        write_json(raw_root / "source_hashes.after.json", after_manifest)
        counts = dict(collections.Counter(result["classification"] for result in results))
        summary = {
            "schema_version": 1,
            "profile": "PROFILE A",
            "run_id": "run_001",
            "page_size": page_size,
            "source_hashes_unchanged": manifest == after_manifest,
            "planned_cases": 30,
            "executed_cases": len(results),
            "counts": {name: counts.get(name, 0) for name in ("PASS", "PARTIAL", "FAIL", "CRASH")},
            "setup_error": 0,
            "results": results,
        }
        write_json(profile_root / "test_results.json", summary)
        print(json.dumps({
            "executed_cases": summary["executed_cases"],
            "counts": summary["counts"],
            "source_hashes_unchanged": summary["source_hashes_unchanged"],
        }, sort_keys=True))
        return 0
    except Exception as error:
        write_text(raw_root / "setup_error.txt", f"{type(error).__name__}: {error}\n")
        print(f"setup error: {error}", file=sys.stderr)
        return 2
    finally:
        shutil.rmtree(temp_root, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
