#!/usr/bin/env python3

from __future__ import annotations

import json
from pathlib import Path
import re
import subprocess
import sys


def main() -> int:
    if len(sys.argv) != 6:
        print(f"usage: {sys.argv[0]} BINARY CASE_ID COUNT OUTPUT_DIR LABEL", file=sys.stderr)
        return 2
    binary = Path(sys.argv[1]).resolve()
    case_id = sys.argv[2]
    count = int(sys.argv[3])
    output_dir = Path(sys.argv[4]).resolve()
    label = sys.argv[5]
    if output_dir.exists():
        print(f"refusing to overwrite {output_dir}", file=sys.stderr)
        return 2
    output_dir.mkdir(parents=True)
    records = []
    for index in range(1, count + 1):
        completed = subprocess.run(
            [str(binary), case_id], text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, timeout=10, check=False,
        )
        stdout_path = output_dir / f"{index:02d}.stdout.raw"
        stderr_path = output_dir / f"{index:02d}.stderr.raw"
        stdout_path.write_text(completed.stdout, encoding="utf-8")
        stderr_path.write_text(completed.stderr, encoding="utf-8")
        records.append({
            "run": index,
            "return_code": completed.returncode,
            "totals": [int(value) for value in re.findall(r"Total size: (\d+) byte", completed.stdout)],
            "foreign_alias": "foreign_free_created_live_alias=1" in completed.stdout,
        })
    summary = {"label": label, "case_id": case_id, "executions": count, "records": records}
    (output_dir / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(summary, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
