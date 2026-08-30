#!/usr/bin/env python3
"""Validate documented reference playthroughs for every shipped level."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOLUTIONS = ROOT / "include" / "level_solutions.h"

PLAYTHROUGHS = (
    "run_knapford_playthrough",
    "run_gordon_hill_playthrough",
    "run_vicarstown_playthrough",
    "run_tidmouth_playthrough",
    "run_wellsworth_playthrough",
    "run_crovan_playthrough",
    "run_callan_playthrough",
    "run_summit_playthrough",
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="Run host session tests")
    args = parser.parse_args()

    header = SOLUTIONS.read_text(encoding="utf-8")

    for name in PLAYTHROUGHS:
        if name not in header:
            print(f"validate_level: missing {name} in {SOLUTIONS}", file=sys.stderr)
            return 1

    if args.check:
        subprocess.run(["make", "-C", str(ROOT / "tests"), "test_session"], check=True)

    print("ok level reference solutions")
    return 0


if __name__ == "__main__":
    sys.exit(main())
