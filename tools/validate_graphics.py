#!/usr/bin/env python3
"""Validate indexed BMP assets for Thomas's Bridge (dimensions + palette)."""

from __future__ import annotations

import os
import struct
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
GFX = os.path.join(ROOT, "graphics")

EXPECTED = {
    "bg_sky.bmp": {"width": 256, "height": 256, "max_colors": 16},
    "bg_hills.bmp": {"width": 256, "height": 256, "max_colors": 16},
    "train.bmp": {"width": 32, "height": 320, "max_colors": 16},
    "portrait.bmp": {"width": 32, "height": 160, "max_colors": 16},
    "node.bmp": {"width": 16, "height": 16, "max_colors": 16},
    "strut.bmp": {"width": 16, "height": 96, "max_colors": 16},
    "scenery_far.bmp": {"width": 64, "height": 32, "max_colors": 16},
}


def read_bmp(path: str) -> tuple[int, int, list[int]]:
    with open(path, "rb") as handle:
        header = handle.read(14)
        if header[:2] != b"BM":
            raise ValueError(f"{path}: not a BMP")
        offset = struct.unpack("<I", header[10:14])[0]
        info = handle.read(40)
        width, height, planes, bpp = struct.unpack("<iihH", info[4:16])
        height = abs(height)
        handle.seek(offset)
        rows = handle.read()
    pixels: list[int] = []
    row_bytes = width
    pad = (4 - (row_bytes % 4)) % 4
    stride = row_bytes + pad
    for y in range(height - 1, -1, -1):
        start = y * stride
        pixels.extend(rows[start : start + row_bytes])
    return width, height, pixels


def validate_file(name: str, rules: dict) -> list[str]:
    path = os.path.join(GFX, name)
    errors: list[str] = []
    if not os.path.isfile(path):
        return [f"missing {name}"]
    width, height, pixels = read_bmp(path)
    if width != rules["width"] or height != rules["height"]:
        errors.append(f"{name}: expected {rules['width']}x{rules['height']}, got {width}x{height}")
    used = sorted(set(pixels))
    if len(used) > rules["max_colors"]:
        errors.append(f"{name}: uses {len(used)} palette indices (max {rules['max_colors']})")
    if 0 in used[1:] if used and used[0] == 0 else used:
        pass
    return errors


def main() -> int:
    errors: list[str] = []
    for name, rules in EXPECTED.items():
        errors.extend(validate_file(name, rules))
    if errors:
        for err in errors:
            print(f"ERROR {err}")
        return 1
    print("Graphics validation passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
