#!/usr/bin/env python3
"""Generate mGBA save files for web playtest walkthroughs.

Embeds save_data v3 (20 bytes) in a 32 KiB GBA SRAM image.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SAVES_DIR = ROOT / "web" / "public" / "saves"

MAGIC = 0x47425254
VERSION = 3
SRAM_SIZE = 32768


def checksum(data: bytes) -> int:
    return sum(data) & 0xFF


def encode_v3(
    max_unlocked_level: int,
    cleared_levels: int,
    unlocked_engines: int,
    packed_stars: int = 0,
    best_costs: tuple[int, int, int] = (0, 0, 0),
    intro_seen_mask: int = 0,
) -> bytes:
    out = bytearray(20)
    struct.pack_into("<I", out, 0, MAGIC)
    out[4] = VERSION
    out[5] = max_unlocked_level & 0xFF
    out[6] = cleared_levels & 0xFF
    out[7] = unlocked_engines & 0xFF
    out[8] = packed_stars & 0xFF
    for index, cost in enumerate(best_costs):
        base = 9 + index * 2
        out[base] = cost & 0xFF
        out[base + 1] = (cost >> 8) & 0xFF
    out[15] = intro_seen_mask & 0xFF
    out[16] = 0
    out[17] = 0
    out[18] = 0
    out[19] = checksum(bytes(out[:19]))
    return bytes(out)


def wrap_sram(save_data: bytes) -> bytes:
    image = bytearray(SRAM_SIZE)
    image[: len(save_data)] = save_data
    return bytes(image)


def gordon_hill_walkthrough_save() -> bytes:
    """Knapford cleared → Gordon's Hill unlocked; skip level intros 0–1."""
    return encode_v3(
        max_unlocked_level=1,
        cleared_levels=0b001,
        unlocked_engines=0b0111,  # Thomas + Percy + Edward (reference playthrough engine)
        intro_seen_mask=0b011,
    )


def all_unlocked_save() -> bytes:
    return encode_v3(
        max_unlocked_level=2,
        cleared_levels=0b111,
        unlocked_engines=0b111111,
        packed_stars=0b111011,  # 3★ on level 0, etc.
        best_costs=(5, 11, 12),
        intro_seen_mask=0b111,
    )


def write_saves() -> None:
    SAVES_DIR.mkdir(parents=True, exist_ok=True)
    specs = {
        "gordon_hill_walkthrough.sav": gordon_hill_walkthrough_save(),
        "all_unlocked.sav": all_unlocked_save(),
    }
    for name, payload in specs.items():
        path = SAVES_DIR / name
        path.write_bytes(wrap_sram(payload))
        print(f"Wrote {path} ({path.stat().st_size} bytes, save_data={len(payload)} bytes)")


def validate() -> int:
    errors: list[str] = []
    expected = {
        "gordon_hill_walkthrough.sav": gordon_hill_walkthrough_save(),
        "all_unlocked.sav": all_unlocked_save(),
    }
    for name, payload in expected.items():
        path = SAVES_DIR / name
        if not path.is_file():
            errors.append(f"missing {path}")
            continue
        data = path.read_bytes()
        if len(data) != SRAM_SIZE:
            errors.append(f"{name}: expected {SRAM_SIZE} bytes, got {len(data)}")
        if data[: len(payload)] != payload:
            errors.append(f"{name}: save_data mismatch")
        if payload[19] != checksum(payload[:19]):
            errors.append(f"{name}: bad checksum")
    if errors:
        for err in errors:
            print(f"FAIL {err}", file=sys.stderr)
        return 1
    print("ok playtest saves")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.check:
        return validate()
    write_saves()
    return validate()


if __name__ == "__main__":
    sys.exit(main())
