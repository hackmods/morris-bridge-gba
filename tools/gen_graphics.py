#!/usr/bin/env python3
"""Thomas's Bridge — professional GBA pixel-art generator.

Art rules: top-left light, 1px outlines on characters/props, 3-tone ramps,
ordered dither for gradients, no scaled-font title on the splash illustration.
"""

from __future__ import annotations

import argparse
import json
import os
import struct
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
GFX = os.path.join(ROOT, "graphics")

# Cohesive 15-bit GBA palette — shared across all sprites
PALETTE = [
    (255, 0, 255),  # 0  transparent
    (24, 24, 32),  # 1  outline / deep shadow
    (248, 248, 240),  # 2  highlight / paper white
    (232, 192, 64),  # 3  gold title
    (208, 56, 48),  # 4  stress / cheek
    (104, 68, 40),  # 5  wood base
    (64, 128, 208),  # 6  engine blue
    (32, 64, 128),  # 7  deep blue / water shadow
    (112, 200, 232),  # 8  water highlight / sky mid
    (192, 48, 48),  # 9  red stripe
    (120, 120, 128),  # 10 metal mid
    (88, 168, 72),  # 11 grass light
    (176, 144, 96),  # 12 sand / wood highlight
    (80, 52, 32),  # 13 cliff shadow
    (48, 104, 48),  # 14 grass shadow
    (168, 208, 248),  # 15 sky light
]

# Template legend keys → palette index
L = {
    ".": 0,
    "1": 1,
    "2": 2,
    "3": 3,
    "4": 4,
    "5": 5,
    "6": 6,
    "7": 7,
    "8": 8,
    "9": 9,
    "a": 10,
    "b": 11,
    "c": 12,
    "d": 13,
    "e": 14,
    "f": 15,
}


def _pad_row(row: bytes) -> bytes:
    pad = (4 - (len(row) % 4)) % 4
    return row + (b"\x00" * pad)


def write_indexed_bmp(path: str, width: int, height: int, pixels: list[int]) -> None:
    assert len(pixels) == width * height
    palette = bytearray()
    for i in range(256):
        r, g, b = PALETTE[i] if i < len(PALETTE) else (0, 0, 0)
        palette += struct.pack("<BBBB", b, g, r, 0)
    rows = bytearray()
    for y in range(height - 1, -1, -1):
        rows += _pad_row(bytes(pixels[y * width : (y + 1) * width]))
    info_size = 40
    pixel_offset = 14 + info_size + 256 * 4
    with open(path, "wb") as handle:
        handle.write(struct.pack("<2sIHHI", b"BM", pixel_offset + len(rows), 0, 0, pixel_offset))
        handle.write(struct.pack("<IiiHHIIiiII", info_size, width, height, 1, 8, 0, len(rows), 0, 0, 256, 0))
        handle.write(palette)
        handle.write(rows)


def fill(w: int, h: int, color: int = 0) -> list[int]:
    return [color] * (w * h)


def put(px: list[int], w: int, x: int, y: int, color: int) -> None:
    if 0 <= x < w and 0 <= y < len(px) // w:
        px[y * w + x] = color


def rect(px: list[int], w: int, x0: int, y0: int, x1: int, y1: int, color: int) -> None:
    for y in range(y0, y1):
        for x in range(x0, x1):
            put(px, w, x, y, color)


def hline(px: list[int], w: int, x0: int, x1: int, y: int, color: int) -> None:
    for x in range(x0, x1):
        put(px, w, x, y, color)


def vline(px: list[int], w: int, x: int, y0: int, y1: int, color: int) -> None:
    for y in range(y0, y1):
        put(px, w, x, y, color)


def blit_map(px: list[int], w: int, ox: int, oy: int, art: str) -> None:
    for y, row in enumerate(art.strip("\n").split("\n")):
        for x, ch in enumerate(row):
            idx = L.get(ch, 0)
            if idx:
                put(px, w, ox + x, oy + y, idx)


def sky_gradient(px: list[int], w: int, h: int, y0: int, y1: int,
                 colors: tuple[int, int, int] = (15, 8, 7)) -> None:
    """Banded illustrated sky — solid ramps, Bayer only on thin horizons."""
    light, mid, dark = colors
    span = max(1, y1 - y0 - 1)
    for y in range(y0, y1):
        t = (y - y0) / span
        if t < 0.40:
            color, alt = light, None
        elif t < 0.50:
            color, alt = light, mid
        elif t < 0.72:
            color, alt = mid, None
        elif t < 0.82:
            color, alt = mid, dark
        else:
            color, alt = dark, None
        for x in range(w):
            if alt is not None and ((x + y * 2) & 3) == 0:
                put(px, w, x, y, alt)
            else:
                put(px, w, x, y, color)


def blit_ellipse(px: list[int], w: int, cx: int, cy: int, rx: int, ry: int, color: int) -> None:
    if rx <= 0 or ry <= 0:
        return
    for dy in range(-ry, ry + 1):
        for dx in range(-rx, rx + 1):
            if (dx / rx) ** 2 + (dy / ry) ** 2 <= 1.0:
                put(px, w, cx + dx, cy + dy, color)


def blit_painted_cloud(px: list[int], w: int, cx: int, cy: int, scale: int = 1) -> None:
    """Layered cloud mass — white puffs with a thin blue underside, not a punched hole."""
    rx, ry = 10 * scale, 5 * scale
    blit_ellipse(px, w, cx, cy, rx, ry, 2)
    blit_ellipse(px, w, cx - 6 * scale, cy + 1, 7 * scale, 4 * scale, 2)
    blit_ellipse(px, w, cx + 6 * scale, cy + 1, 6 * scale, 4 * scale, 2)
    blit_ellipse(px, w, cx, cy - 2 * scale, 5 * scale, 3 * scale, 2)
    for dx in range(-rx + 2, rx - 1):
        put(px, w, cx + dx, cy + ry, 8)
        if abs(dx) < rx - 3:
            put(px, w, cx + dx, cy + ry - 1, 8)


def blit_sun(px: list[int], w: int, cx: int, cy: int, radius: int = 8) -> None:
    for dy in range(-radius - 3, radius + 4):
        for dx in range(-radius - 3, radius + 4):
            d2 = dx * dx + dy * dy
            if d2 <= radius * radius:
                put(px, w, cx + dx, cy + dy, 3)
            elif d2 <= (radius + 2) * (radius + 2):
                put(px, w, cx + dx, cy + dy, 2)


def draw_hatt_figure(px: list[int], w: int, ox: int, oy: int, angry: bool = False) -> None:
    """32×32 Sir Topham Hatt — top hat, face, coat, not a color blob."""
    band = 9 if angry else 3
    coat = 1
    coat_hi = 10
    skin = 12
    skin_hi = 3

    # Top hat crown
    rect(px, w, ox + 10, oy + 1, ox + 22, oy + 11, 1)
    rect(px, w, ox + 11, oy + 2, ox + 21, oy + 10, coat_hi)
    hline(px, w, ox + 11, ox + 21, oy + 2, 2)
    rect(px, w, ox + 11, oy + 7, ox + 21, oy + 10, band)
    hline(px, w, ox + 11, ox + 21, oy + 7, 1)
    # Brim
    rect(px, w, ox + 6, oy + 10, ox + 26, oy + 13, 1)
    rect(px, w, ox + 7, oy + 11, ox + 25, oy + 12, coat_hi)

    # Head
    rect(px, w, ox + 9, oy + 13, ox + 23, oy + 23, 1)
    rect(px, w, ox + 10, oy + 14, ox + 22, oy + 22, skin)
    rect(px, w, ox + 11, oy + 14, ox + 21, oy + 16, skin_hi)
    # Eyes
    put(px, w, ox + 13, oy + 16, 1)
    put(px, w, ox + 18, oy + 16, 1)
    put(px, w, ox + 14, oy + 16, 2)
    put(px, w, ox + 19, oy + 16, 2)
    if angry:
        hline(px, w, ox + 12, ox + 15, oy + 15, 1)
        hline(px, w, ox + 17, ox + 20, oy + 15, 1)
        put(px, w, ox + 12, oy + 16, 1)
        put(px, w, ox + 19, oy + 16, 1)
        hline(px, w, ox + 14, ox + 18, oy + 20, 1)
        put(px, w, ox + 13, oy + 19, 1)
        put(px, w, ox + 18, oy + 19, 1)
        put(px, w, ox + 12, oy + 18, 4)
        put(px, w, ox + 19, oy + 18, 4)
    else:
        put(px, w, ox + 12, oy + 18, 4)
        put(px, w, ox + 19, oy + 18, 4)
        hline(px, w, ox + 14, ox + 18, oy + 19, 1)
        put(px, w, ox + 13, oy + 20, 1)
        put(px, w, ox + 18, oy + 20, 1)

    # Mustache
    hline(px, w, ox + 13, ox + 19, oy + 18, 1)
    put(px, w, ox + 12, oy + 17, 1)
    put(px, w, ox + 19, oy + 17, 1)

    # Coat + shirt + tie
    rect(px, w, ox + 8, oy + 22, ox + 24, oy + 30, coat)
    rect(px, w, ox + 9, oy + 23, ox + 23, oy + 29, coat_hi)
    rect(px, w, ox + 14, oy + 22, ox + 18, oy + 28, 2)
    vline(px, w, ox + 15, oy + 23, oy + 27, 6 if not angry else 9)
    vline(px, w, ox + 16, oy + 23, oy + 27, 6 if not angry else 9)
    put(px, w, ox + 15, oy + 23, 7)
    # Collar
    put(px, w, ox + 13, oy + 22, 2)
    put(px, w, ox + 18, oy + 22, 2)
    # Trousers / shoes
    rect(px, w, ox + 10, oy + 29, ox + 15, oy + 32, coat_hi)
    rect(px, w, ox + 17, oy + 29, ox + 22, oy + 32, coat_hi)
    hline(px, w, ox + 10, ox + 15, oy + 31, 1)
    hline(px, w, ox + 17, ox + 22, oy + 31, 1)


def horizon_haze(px: list[int], w: int, y0: int, y1: int, color: int, strength: int = 2) -> None:
    """Soft horizon band — 2–3 px fade into hills/water."""
    for y in range(y0, y1):
        t = (y - y0) / max(1, y1 - y0 - 1)
        for x in range(w):
            if (x + y) % strength == 0:
                put(px, w, x, y, color)


def blit_tree_silhouette(px: list[int], w: int, ox: int, base_y: int, trunk: int, canopy: int) -> None:
    """Simple 5×8 tree for meadow hills."""
    vline(px, w, ox + 2, base_y - 6, base_y, trunk)
    for dy, span in enumerate((1, 3, 5, 3, 1)):
        y = base_y - 7 - dy
        x0 = ox + 2 - span // 2
        hline(px, w, x0, x0 + span, y, canopy)


def blit_rock_outcrop(px: list[int], w: int, ox: int, base_y: int, light: int, shadow: int) -> None:
    """Jagged cliff nub for Gordon's Hill."""
    for dy in range(6):
        span = 2 + (dy % 3)
        y = base_y - dy
        x0 = ox + 1 - span // 2
        c = light if dy < 3 else shadow
        hline(px, w, x0, x0 + span, y, c)
    put(px, w, ox, base_y - 6, shadow)


def grass_tuft(px: list[int], w: int, x: int, y: int, light: int, shadow: int) -> None:
    """2–3 px meadow grass along bank top."""
    put(px, w, x, y, light)
    put(px, w, x - 1, y + 1, shadow)
    put(px, w, x + 1, y + 1, shadow)


# Per-level BG themes — knapford (0), hill (1), viaduct (2).
BG_THEMES: list[dict] = [
    {
        "name": "knapford",
        "sky": (15, 8, 7),
        "hill_light": 11,
        "hill_shadow": 14,
        "ground_top": 11,
        "ground_mid": 14,
        "ground_fill": 13,
        "ground_hi": 12,
        "water_dark": 7,
        "water_light": 8,
        "water_spark": 2,
        "rock": (13, 5, 12),
    },
    {
        "name": "hill",
        "sky": (3, 8, 7),
        "hill_light": 14,
        "hill_shadow": 13,
        "ground_top": 13,
        "ground_mid": 14,
        "ground_fill": 13,
        "ground_hi": 12,
        "water_dark": 7,
        "water_light": 7,
        "water_spark": 1,
        "rock": (13, 5, 12),
    },
    {
        "name": "viaduct",
        "sky": (10, 1, 7),
        "hill_light": 10,
        "hill_shadow": 1,
        "ground_top": 10,
        "ground_mid": 13,
        "ground_fill": 12,
        "ground_hi": 1,
        "water_dark": 1,
        "water_light": 10,
        "water_spark": 7,
        "rock": (10, 7, 15),
    },
]


def panel_box(px: list[int], w: int, x0: int, y0: int, x1: int, y1: int) -> None:
    rect(px, w, x0, y0, x1, y1, 7)
    rect(px, w, x0 + 1, y0 + 1, x1 - 1, y1 - 1, 2)
    rect(px, w, x0 + 2, y0 + 2, x1 - 2, y1 - 2, 7)
    rect(px, w, x0 + 3, y0 + 3, x1 - 3, y1 - 3, 2)


FONT = {
    "A": ["01110", "10001", "11111", "10001", "10001"],
    "B": ["11110", "10001", "11110", "10001", "11110"],
    "C": ["01111", "10000", "10000", "10000", "01111"],
    "D": ["11110", "10001", "10001", "10001", "11110"],
    "E": ["11111", "10000", "11110", "10000", "11111"],
    "F": ["11111", "10000", "11110", "10000", "10000"],
    "G": ["01111", "10000", "10111", "10001", "01110"],
    "H": ["10001", "10001", "11111", "10001", "10001"],
    "I": ["11111", "00100", "00100", "00100", "11111"],
    "J": ["11111", "00100", "00100", "10100", "01100"],
    "K": ["10001", "10010", "11100", "10010", "10001"],
    "L": ["10000", "10000", "10000", "10000", "11111"],
    "M": ["10001", "11011", "10101", "10001", "10001"],
    "N": ["10001", "11001", "10101", "10011", "10001"],
    "O": ["01110", "10001", "10001", "10001", "01110"],
    "P": ["11110", "10001", "11110", "10000", "10000"],
    "R": ["11110", "10001", "11110", "10010", "10001"],
    "S": ["01111", "10000", "01110", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100"],
    "U": ["10001", "10001", "10001", "10001", "01110"],
    "V": ["10001", "10001", "10001", "01010", "01010"],
    "W": ["10001", "10001", "10101", "10101", "01010"],
    "X": ["10001", "01010", "00100", "01010", "10001"],
    "Y": ["10001", "01010", "00100", "00100", "00100"],
    "'": ["00100", "00100", "01000", "00000", "00000"],
    "!": ["00100", "00100", "00100", "00000", "00100"],
    ".": ["00000", "00000", "00000", "00000", "00100"],
    " ": ["00000", "00000", "00000", "00000", "00000"],
    "0": ["01110", "10001", "10001", "10001", "01110"],
    "1": ["00100", "01100", "00100", "00100", "01110"],
    "2": ["01110", "10001", "00110", "01000", "11111"],
    "3": ["11110", "00001", "01110", "00001", "11110"],
    "4": ["10010", "10010", "11111", "00010", "00010"],
    "5": ["11111", "10000", "11110", "00001", "11110"],
    "6": ["01110", "10000", "11110", "10001", "01110"],
    "7": ["11111", "00001", "00010", "00100", "01000"],
    "8": ["01110", "10001", "01110", "10001", "01110"],
    "9": ["01110", "10001", "01111", "00001", "01110"],
}


def blit_text(px: list[int], w: int, text: str, ox: int, oy: int, color: int, shadow: int = 1,
              advance: int = 6) -> None:
    x = ox
    for ch in text:
        glyph = FONT.get(ch, FONT[" "])
        for gy, row in enumerate(glyph):
            for gx, bit in enumerate(row):
                if bit == "1":
                    put(px, w, x + gx + 1, oy + gy + 1, shadow)
                    put(px, w, x + gx, oy + gy, color)
        x += advance


def center_text(px: list[int], w: int, text: str, y: int, color: int, shadow: int = 1,
                 advance: int = 6) -> None:
    tw = len(text) * advance - 1
    blit_text(px, w, text, max(0, (w - tw) // 2), y, color, shadow, advance)


# ---------------------------------------------------------------------------
# Engines and coach — 32×32 side view facing right, wheels on the bottom row.
# Drawn from parts so every engine shares the same proportions and face.
# ---------------------------------------------------------------------------

# name, kind, (body, shade, highlight), lining, dome, number, funnel column,
# wheels front-to-back as (centre x, size): D driving 9 px, d driving 7 px,
# s carrying 5 px. Rows follow engine::Id, not the roster.
ENGINE_ART = (
    ("Thomas", "tank", (6, 7, 8), 9, 3, "1", 22, ((23, "D"), (15, "D"), (7, "D"))),
    ("Percy", "saddle", (11, 14, 2), 9, 3, "6", 21, ((20, "D"), (9, "D"))),
    ("Edward", "tender", (6, 7, 8), 9, 3, "2", 22, ((27, "s"), (22, "s"), (14, "D"), (6, "D"))),
    ("James", "tender", (9, 13, 4), 3, 3, "5", 22, ((28, "s"), (21, "D"), (13, "D"), (5, "D"))),
    ("Gordon", "express", (6, 7, 8), 9, 6, "4", 23,
     ((29, "s"), (25, "s"), (19, "d"), (13, "d"), (7, "d"), (2, "s"))),
    ("Henry", "tender", (11, 14, 2), 9, 3, "3", 22,
     ((29, "s"), (25, "s"), (19, "d"), (12, "d"), (5, "d"))),
)

# 3×5 digits for cab-side and tank-side numbers.
TINY_DIGITS = {
    "1": ["010", "110", "010", "010", "111"],
    "2": ["111", "001", "111", "100", "111"],
    "3": ["111", "001", "011", "001", "111"],
    "4": ["101", "101", "111", "001", "001"],
    "5": ["111", "100", "111", "001", "111"],
    "6": ["111", "100", "111", "101", "111"],
}


def blit_tiny_digit(px: list[int], w: int, digit: str, ox: int, oy: int, color: int) -> None:
    for gy, row in enumerate(TINY_DIGITS[digit]):
        for gx, bit in enumerate(row):
            if bit == "1":
                put(px, w, ox + gx, oy + gy, color)


def disc(px: list[int], w: int, cx2: int, cy2: int, r2: int, color: int) -> None:
    """Filled circle from doubled centre/diameter so even sizes stay symmetric."""
    for y in range((cy2 - r2) // 2 - 1, (cy2 + r2) // 2 + 2):
        for x in range((cx2 - r2) // 2 - 1, (cx2 + r2) // 2 + 2):
            dx = 2 * x + 1 - cx2
            dy = 2 * y + 1 - cy2
            if dx * dx + dy * dy <= r2 * r2:
                put(px, w, x, y, color)


WHEEL_DIAMETER = {"D": 9, "d": 7, "s": 5}


def draw_wheel(px: list[int], w: int, cx: int, y0: int, frame: int, size: str = "D") -> None:
    """A wheel with its bottom on row y0 + 31. Driving wheels show a turning spoke."""
    d = WHEEL_DIAMETER[size]
    top = y0 + 32 - d
    centre2 = 2 * top + d
    disc(px, w, 2 * cx + 1, centre2, d, 1)
    disc(px, w, 2 * cx + 1, centre2, d - 2, 10)
    if size == "s":
        put(px, w, cx, top + 2, 1)
        return
    disc(px, w, 2 * cx + 1, centre2, 3 if size == "D" else 1, 1)
    reach = d // 2 - 1
    mid = top + d // 2
    # Two spoke positions make the wheels turn between frames.
    if frame == 0:
        put(px, w, cx, mid - reach, 2)
        put(px, w, cx, mid + reach, 13)
    else:
        put(px, w, cx - reach, mid, 2)
        put(px, w, cx + reach, mid, 13)


def draw_face(px: list[int], w: int, cx: int, cy: int) -> None:
    """The smokebox face: grey-white disc, two eyes, a smile."""
    disc(px, w, 2 * cx + 1, 2 * cy + 1, 11, 1)
    disc(px, w, 2 * cx + 1, 2 * cy + 1, 9, 2)
    # Soft shade under the chin, away from the top-left light.
    for x in range(cx - 2, cx + 3):
        put(px, w, x, cy + 4, 10)
    put(px, w, cx + 3, cy + 3, 10)
    put(px, w, cx - 2, cy - 1, 1)
    put(px, w, cx + 2, cy - 1, 1)
    put(px, w, cx - 2, cy - 2, 10)
    put(px, w, cx + 2, cy - 2, 10)
    put(px, w, cx - 2, cy + 1, 1)
    hline(px, w, cx - 1, cx + 2, cy + 2, 1)
    put(px, w, cx + 2, cy + 1, 1)
    put(px, w, cx - 3, cy + 1, 4)
    put(px, w, cx + 3, cy + 1, 4)


def draw_engine(px: list[int], w: int, y0: int, kind: str, colors: tuple[int, int, int],
                lining: int, dome: int, number: str, funnel_x: int, wheels: tuple,
                frame: int) -> None:
    body, shade, light = colors
    big = kind == "express"
    boiler_top = y0 + (10 if big else 12)

    # Frames, running plate and buffer beams.
    rect(px, w, 2, y0 + 23, 30, y0 + 25, 1)
    hline(px, w, 2, 30, y0 + 22, lining)
    rect(px, w, 29, y0 + 20, 31, y0 + 25, 9)
    put(px, w, 31, y0 + 22, 10)
    put(px, w, 31, y0 + 23, 10)
    rect(px, w, 0, y0 + 21, 2, y0 + 25, 9)

    # Boiler with a lit top edge and shaded belly.
    rect(px, w, 9, boiler_top, 27, y0 + 22, body)
    hline(px, w, 10, 27, boiler_top, light)
    hline(px, w, 9, 27, y0 + 21, shade)
    hline(px, w, 9, 27, y0 + 20, shade)
    for band in (13, 18):
        vline(px, w, band, boiler_top + 1, y0 + 20, shade)

    # Smokebox and funnel in black.
    rect(px, w, 24, boiler_top - 1, 28, y0 + 22, 1)
    vline(px, w, 24, boiler_top, y0 + 21, 10)
    funnel_top = y0 + (6 if big else 4)
    rect(px, w, funnel_x, funnel_top, funnel_x + 3, boiler_top - 1, 1)
    vline(px, w, funnel_x, funnel_top + 1, boiler_top - 1, 10)
    hline(px, w, funnel_x - 1, funnel_x + 4, funnel_top, 1)

    # Dome.
    dome_x = 15 if not big else 14
    rect(px, w, dome_x, boiler_top - 3, dome_x + 4, boiler_top, dome)
    hline(px, w, dome_x + 1, dome_x + 3, boiler_top - 4, dome)
    put(px, w, dome_x + 1, boiler_top - 3, 2)
    hline(px, w, dome_x, dome_x + 4, boiler_top, 1)

    # Cab: roof, body, window.
    cab_right = 10
    rect(px, w, 3, y0 + 7, cab_right, y0 + 22, body)
    vline(px, w, 3, y0 + 7, y0 + 21, shade)
    vline(px, w, cab_right - 1, y0 + 8, boiler_top, shade)
    rect(px, w, 2, y0 + 5, cab_right + 1, y0 + 7, 1)
    hline(px, w, 3, cab_right, y0 + 5, 10)
    rect(px, w, 5, y0 + 9, 8, y0 + 13, 1)
    rect(px, w, 5, y0 + 9, 8, y0 + 12, 15)
    put(px, w, 5, y0 + 9, 2)

    if kind == "tank":
        # Side tanks hide the lower boiler; the number goes on the tank.
        rect(px, w, 10, y0 + 13, 21, y0 + 22, body)
        hline(px, w, 10, 21, y0 + 13, light)
        hline(px, w, 10, 21, y0 + 21, shade)
        rect(px, w, 11, y0 + 14, 20, y0 + 15, lining)
        rect(px, w, 11, y0 + 15, 20, y0 + 20, body)
        hline(px, w, 11, 20, y0 + 20, lining)
        vline(px, w, 11, y0 + 14, y0 + 20, lining)
        vline(px, w, 19, y0 + 14, y0 + 20, lining)
        blit_tiny_digit(px, w, number, 14, y0 + 15, 3)
        rect(px, w, 1, y0 + 12, 3, y0 + 22, body)
        hline(px, w, 1, 3, y0 + 11, 1)
    elif kind == "saddle":
        # Saddle tank wraps over the boiler top.
        rect(px, w, 11, y0 + 10, 22, y0 + 17, body)
        hline(px, w, 12, 21, y0 + 9, body)
        hline(px, w, 12, 21, y0 + 9, light)
        hline(px, w, 11, 22, y0 + 17, shade)
        hline(px, w, 12, 21, y0 + 11, lining)
        blit_tiny_digit(px, w, number, 15, y0 + 12, 3)
        rect(px, w, 1, y0 + 13, 3, y0 + 22, body)
        hline(px, w, 1, 3, y0 + 12, 1)
    else:
        # Tender engines: wheel splashers, number on the cab side.
        for cx, size in wheels:
            if size != "s":
                hline(px, w, cx - 3, cx + 4, y0 + 20, lining)
                hline(px, w, cx - 4, cx + 5, y0 + 21, body)
        blit_tiny_digit(px, w, number, 5, y0 + 15, 3)
        hline(px, w, 4, 9, y0 + 21, lining)
        rect(px, w, 0, y0 + 14, 3, y0 + 22, body)
        hline(px, w, 0, 3, y0 + 13, lining)
        vline(px, w, 0, y0 + 14, y0 + 21, shade)

    # Wheels, then the coupling rod across the driving wheels only.
    for cx, size in wheels:
        draw_wheel(px, w, cx, y0, frame, size)
    drivers = [cx for cx, size in wheels if size != "s"]
    rod_y = y0 + (27 if frame == 0 else 28)
    hline(px, w, min(drivers), max(drivers) + 1, rod_y, 12)

    draw_face(px, w, 26, boiler_top + 5)

    if frame == 1:
        # Chuff: a puff of steam off the funnel, drifting back.
        for dx, dy, color in ((0, -2, 2), (1, -3, 2), (-1, -3, 15), (2, -2, 15),
                              (-2, -4, 2), (-3, -4, 15), (0, -4, 2)):
            if funnel_top + dy >= y0:
                put(px, w, funnel_x + 1 + dx, funnel_top + dy, color)
    else:
        put(px, w, funnel_x + 1, funnel_top - 1, 15)


def make_train() -> None:
    w, fh = 32, 32
    px = fill(w, fh * 2 * len(ENGINE_ART))
    for i, (_name, kind, colors, lining, dome, number, funnel_x, wheels) in enumerate(ENGINE_ART):
        for frame in range(2):
            draw_engine(px, w, (i * 2 + frame) * fh, kind, colors, lining, dome, number,
                        funnel_x, wheels, frame)
    write_indexed_bmp(os.path.join(GFX, "train.bmp"), w, fh * 2 * len(ENGINE_ART), px)


def draw_tender(px: list[int], w: int, y0: int, colors: tuple[int, int, int], lining: int) -> None:
    body, shade, light = colors
    rect(px, w, 2, y0 + 23, 30, y0 + 25, 1)
    rect(px, w, 3, y0 + 12, 29, y0 + 23, body)
    hline(px, w, 3, 29, y0 + 12, light)
    hline(px, w, 3, 29, y0 + 22, shade)
    vline(px, w, 3, y0 + 13, y0 + 22, shade)
    # Lined panel.
    hline(px, w, 6, 26, y0 + 15, lining)
    hline(px, w, 6, 26, y0 + 20, lining)
    vline(px, w, 6, y0 + 15, y0 + 21, lining)
    vline(px, w, 25, y0 + 15, y0 + 21, lining)
    # Coal, heaped toward the engine.
    for x in range(5, 28):
        heap = 2 + (x - 5) // 8 + (_noise(x, 2) % 2)
        for y in range(y0 + 12 - heap, y0 + 12):
            put(px, w, x, y, 1 if _noise(x, y) > 60 else 10)
    put(px, w, 1, y0 + 21, 10)
    put(px, w, 1, y0 + 22, 10)
    put(px, w, 30, y0 + 21, 10)
    put(px, w, 30, y0 + 22, 10)
    for cx in (8, 16, 24):
        draw_wheel(px, w, cx, y0, 0, "d")


def draw_coach(px: list[int], w: int, y0: int, body: int, band: int, lining: int,
               windows: tuple[int, ...], door: int, bogies: bool) -> None:
    rect(px, w, 1, y0 + 10, 31, y0 + 12, 1)
    hline(px, w, 2, 29, y0 + 10, 10)
    rect(px, w, 1, y0 + 12, 31, y0 + 24, body)
    rect(px, w, 2, y0 + 13, 30, y0 + 18, band)
    for x in windows:
        rect(px, w, x, y0 + 14, x + 4, y0 + 18, 1)
        rect(px, w, x, y0 + 14, x + 4, y0 + 17, 15)
        put(px, w, x, y0 + 14, 2)
    rect(px, w, door, y0 + 13, door + 4, y0 + 23, 13)
    put(px, w, door + 2, y0 + 18, 3)
    hline(px, w, 2, 29, y0 + 19, lining)
    hline(px, w, 1, 30, y0 + 23, 13)
    vline(px, w, 1, y0 + 12, y0 + 23, 13)
    rect(px, w, 2, y0 + 24, 30, y0 + 26, 1)
    for bx in (0, 31):
        put(px, w, bx, y0 + 21, 10)
        put(px, w, bx, y0 + 22, 10)
    for cx in ((6, 11, 20, 25) if bogies else (8, 23)):
        draw_wheel(px, w, cx, y0, 0, "d" if bogies else "D")


def draw_truck(px: list[int], w: int, y0: int) -> None:
    """A Troublesome Truck: plank wagon, heaped load, and a scowl on the end."""
    rect(px, w, 3, y0 + 23, 29, y0 + 25, 1)
    rect(px, w, 3, y0 + 13, 29, y0 + 23, 10)
    for y in (16, 19, 22):
        hline(px, w, 3, 29, y0 + y, 13)
    for x in (3, 11, 20, 28):
        vline(px, w, x, y0 + 13, y0 + 23, 1)
    hline(px, w, 3, 29, y0 + 13, 2)
    # Load.
    for x in range(5, 27):
        heap = 1 + (3 - abs(x - 15) // 4) + (_noise(x, 9) % 2)
        for y in range(y0 + 13 - heap, y0 + 13):
            put(px, w, x, y, 5 if _noise(x, y) > 110 else 13)
    # The face, on the leading end.
    rect(px, w, 21, y0 + 14, 28, y0 + 22, 2)
    put(px, w, 22, y0 + 16, 1)
    put(px, w, 26, y0 + 16, 1)
    put(px, w, 22, y0 + 15, 1)
    put(px, w, 23, y0 + 15, 10)
    put(px, w, 26, y0 + 15, 1)
    put(px, w, 25, y0 + 15, 10)
    hline(px, w, 23, 26, y0 + 19, 1)
    put(px, w, 22, y0 + 20, 1)
    put(px, w, 26, y0 + 20, 1)
    put(px, w, 2, y0 + 21, 10)
    put(px, w, 29, y0 + 21, 10)
    for cx in (9, 22):
        draw_wheel(px, w, cx, y0, 0, "D")


# Frames follow engine::CarKind.
CAR_FRAMES = ("tender_blue", "tender_green", "tender_red", "annie", "clarabel", "coach", "truck")


def make_car() -> None:
    """Everything that rolls behind an engine — one 32×32 frame per engine::CarKind."""
    w, fh = 32, 32
    px = fill(w, fh * len(CAR_FRAMES))
    for i, kind in enumerate(CAR_FRAMES):
        y0 = i * fh
        if kind == "tender_blue":
            draw_tender(px, w, y0, (6, 7, 8), 9)
        elif kind == "tender_green":
            draw_tender(px, w, y0, (11, 14, 2), 9)
        elif kind == "tender_red":
            draw_tender(px, w, y0, (9, 13, 4), 3)
        elif kind == "annie":
            draw_coach(px, w, y0, 5, 12, 3, (3, 9, 19, 25), 14, False)
        elif kind == "clarabel":
            # Clarabel carries the guard: a blank panel where Annie has a window.
            draw_coach(px, w, y0, 5, 12, 3, (9, 19, 25), 3, False)
        elif kind == "coach":
            draw_coach(px, w, y0, 14, 2, 3, (3, 9, 19, 25), 14, True)
        else:
            draw_truck(px, w, y0)
    write_indexed_bmp(os.path.join(GFX, "car.bmp"), w, fh * len(CAR_FRAMES), px)


def make_portrait() -> None:
    """Engine-select portraits: the face up close, in the engine's own livery."""
    w, fh = 32, 32
    px = fill(w, fh * len(ENGINE_ART))
    for i, (_name, _kind, colors, lining, _dome, number, _funnel_x, _wheels) in enumerate(ENGINE_ART):
        body, shade, light = colors
        y0 = i * fh
        rect(px, w, 1, y0 + 1, 31, y0 + 31, 1)
        rect(px, w, 2, y0 + 2, 30, y0 + 30, body)
        rect(px, w, 2, y0 + 2, 30, y0 + 4, light)
        rect(px, w, 2, y0 + 20, 30, y0 + 22, shade)
        # Smokebox ring, then the face.
        disc(px, w, 32, 2 * (y0 + 12) + 1, 19, 1)
        disc(px, w, 32, 2 * (y0 + 12) + 1, 15, 2)
        for x in range(12, 21):
            put(px, w, x, y0 + 19, 10)
        for ex in (12, 18):
            rect(px, w, ex, y0 + 8, ex + 3, y0 + 12, 1)
            put(px, w, ex + 1, y0 + 9, 2)
            hline(px, w, ex, ex + 2, y0 + 6, 1)
        put(px, w, 11, y0 + 14, 4)
        put(px, w, 21, y0 + 14, 4)
        put(px, w, 12, y0 + 15, 1)
        hline(px, w, 13, 19, y0 + 16, 1)
        put(px, w, 19, y0 + 15, 1)
        # Number plate and name.
        rect(px, w, 25, y0 + 3, 30, y0 + 10, 1)
        rect(px, w, 26, y0 + 4, 29, y0 + 9, lining if lining != 3 else 13)
        blit_tiny_digit(px, w, number, 26, y0 + 4, 3)
        rect(px, w, 2, y0 + 22, 30, y0 + 30, 1)
        center_text(px, w, f"NO {number}", y0 + 24, 3, 13)
    write_indexed_bmp(os.path.join(GFX, "portrait.bmp"), w, fh * len(ENGINE_ART), px)


# ---------------------------------------------------------------------------
# Per-level terrain — one 256×256 ground layer per track, drawn from the same
# anchor table the game uses, so the banks end where the bank anchors are and
# every rock, crag, ledge and pylon stands under its anchor.
#
# Screen (x, y) maps to bitmap (x + 128, y + 128); the game shows the layer
# unscrolled.
# ---------------------------------------------------------------------------

LEVEL_SOURCE = os.path.join(ROOT, "src", "content", "level.cpp")

# What each extra anchor is standing on, in level.cpp seed order (after the
# two banks), plus scenery on the banks: (kind, x). parse_levels() checks the
# count against level.cpp so art cannot drift from the anchors.
TERRAIN = (
    ("knapford", ("rock",), (("station", 84), ("signal", -62), ("tree", -104))),
    ("gordons_hill", ("crag", "crag"), (("signal", -104), ("tree", 104))),
    ("culdee_fell", (), (("signal", -82), ("tree", 86), ("tree", 106))),
    ("tidmouth", (), (("shed", 84), ("signal", -70), ("tree", -108))),
    ("wellsworth", ("ledge", "ledge"), (("signal", -78), ("tree", 100))),
    ("crovan", ("rock",), (("signal", -106), ("lamp", 106))),
    ("callan", ("tower", "tower"), (("signal", -96), ("tree", 104))),
    ("vicarstown", ("rock",), (("signal", -110), ("lamp", 108))),
)


def parse_levels() -> list[dict]:
    """Read anchors, theme, deck and water height for every level from level.cpp."""
    import re

    with open(LEVEL_SOURCE, encoding="utf-8") as handle:
        source = handle.read()

    seeds: dict[str, list[tuple[int, int]]] = {}
    for name, body in re.findall(r"constexpr Seed (\w+)\[\] = \{(.*?)\};", source, re.S):
        seeds[name] = [(int(x), int(y)) for x, y in re.findall(r"\{\s*(-?\d+),\s*(-?\d+),", body)]

    levels = []
    for name, theme, deck, water in re.findall(
            r"\{ SEEDS\((\w+)\), (\d+), engine::Id::\w+, (-?\d+), \d+,\s*(-?\d+),", source):
        levels.append({
            "seeds": seeds[name],
            "theme": int(theme),
            "deck": int(deck),
            "water": int(water),
        })

    if len(levels) != len(TERRAIN):
        raise ValueError(f"level.cpp has {len(levels)} levels, TERRAIN has {len(TERRAIN)}")

    for level, (name, kinds, _props) in zip(levels, TERRAIN):
        if len(level["seeds"]) - 2 != len(kinds):
            raise ValueError(f"{name}: {len(level['seeds']) - 2} extra anchors in level.cpp, "
                             f"{len(kinds)} in TERRAIN")
        if level["seeds"][0][1] != level["deck"]:
            raise ValueError(f"{name}: the start bank is not at deck height")

    return levels


def _noise(x: int, y: int) -> int:
    """Cheap deterministic speckle for rock texture."""
    n = (x * 73856093) ^ (y * 19349663)
    return (n >> 4) & 0xFF


# Nothing is drawn below this screen row; make_bg_terrain sets it to the river
# surface so the animated water layer behind shows through.
_terrain_floor = 128


def sput(px: list[int], x: int, y: int, color: int) -> None:
    if y <= _terrain_floor:
        put(px, 256, x + 128, y + 128, color)


def srect(px: list[int], x0: int, y0: int, x1: int, y1: int, color: int) -> None:
    rect(px, 256, x0 + 128, y0 + 128, x1 + 128, y1 + 128, color)


def rock_color(x: int, y: int, top: int, theme: dict) -> int:
    """Chunky rock: staggered blocks in two tones, lit on top, cracked beneath."""
    base, facet, light = theme["rock"]
    depth = y - top
    course = depth // 7
    row_in = depth % 7
    shift = (_noise(course, 11) % 9) + (5 if course % 2 else 0)
    block = (x + shift + 128) // 13
    col_in = (x + shift + 128) % 13
    tone = _noise(block, course * 3 + 1)
    if row_in == 6 or col_in == 0:
        return 1 if tone > 150 else base
    if row_in == 0 and tone > 60 and col_in < 10:
        return light
    if tone > 170:
        return facet
    if tone < 40 and row_in > 2 and col_in > 6:
        return facet
    return base


def draw_bank(px: list[int], side: int, edge_x: int, deck: int, water: int, theme: dict) -> None:
    """One bank: rails on ballast, a turf lip, then a cliff face down into the river.

    side is -1 for the left bank (cliff faces right, in shadow) and +1 for the right.
    """
    rail_y = deck - 5
    far = -128 if side < 0 else 127
    lo, hi = (far, edge_x + 3) if side < 0 else (edge_x - 3, far)

    for x in range(lo, hi + 1):
        from_edge = (hi - x) if side < 0 else (x - lo)
        # The face steps back in ragged shelves as it drops.
        for y in range(rail_y + 5, 128):
            depth = y - rail_y
            shelf = min(6, depth // 9) + (_noise(depth // 5, side) % 3)
            if from_edge < shelf and depth > 7:
                continue
            is_edge = from_edge == shelf or (from_edge == 0 and depth <= 7)
            color = rock_color(x, y, rail_y, theme)
            if is_edge:
                color = theme["rock"][2] if side > 0 else 1
            sput(px, x, y, color)

        # Track bed.
        sput(px, x, rail_y, 2 if x % 8 == 0 else 10)
        sput(px, x, rail_y + 1, 13 if x % 4 < 2 else 12)
        sput(px, x, rail_y + 2, 12)
        sput(px, x, rail_y + 3, theme["ground_top"])
        sput(px, x, rail_y + 4, theme["ground_mid"] if _noise(x, 7) > 90 else theme["ground_top"])

    # Wet rock where the cliff meets the river.
    for x in range(lo, hi + 1):
        if px[(water + 128) * 256 + x + 128] != 0:
            sput(px, x, water, 1)
            sput(px, x, water - 1, theme["water_spark"] if _noise(x, 3) > 150 else 1)


def draw_rock(px: list[int], ax: int, ay: int, water: int, theme: dict) -> None:
    """A rock standing out of the river with a flat top just under its anchor."""
    top = ay + 3
    for y in range(top, 128):
        half = 9 + min(14, (y - top) // 2) + (_noise(y // 3, ax) % 2)
        for x in range(ax - half, ax + half + 1):
            color = rock_color(x, y, top, theme)
            if x == ax - half:
                color = theme["rock"][2]
            elif x == ax + half:
                color = 1
            elif y == top:
                color = theme["rock"][2]
            sput(px, x, y, color)
        if y == water:
            for x in range(ax - half, ax + half + 1):
                sput(px, x, y, 1)


def draw_crag(px: list[int], ax: int, ay: int, theme: dict) -> None:
    """A tall rock stack rising behind the line, its head at the anchor."""
    top = ay + 3
    lean = -1 if ax < 0 else 1
    for y in range(top - 5, 128):
        depth = y - top
        half = 5 + max(0, depth) // 7 + (_noise(y // 4, ax) % 2)
        if depth < 0:
            half = 2 + (depth + 5)
        cx = ax + lean * max(0, depth) // 14
        for x in range(cx - half, cx + half + 1):
            color = rock_color(x, y, top, theme)
            if x == cx - half:
                color = theme["rock"][2]
            elif x == cx + half:
                color = 1
            sput(px, x, y, color)
    # A scrap of turf on the head.
    for x in range(ax - 4, ax + 5):
        sput(px, x, top - 1, theme["ground_top"] if abs(x - ax) < 4 else theme["ground_mid"])


def draw_ledge(px: list[int], ax: int, ay: int, theme: dict) -> None:
    """A shelf jutting from the cutting wall, corbelled underneath."""
    side = -1 if ax < 0 else 1
    top = ay + 3
    for step in range(10):
        reach = 14 - step - step // 2
        y = top + step
        for i in range(-6, reach):
            x = ax - side * i
            color = rock_color(x, y, top, theme)
            if step == 0:
                color = theme["rock"][2]
            elif i == reach - 1:
                color = 1
            sput(px, x, y, color)


def draw_tower(px: list[int], ax: int, ay: int, base: int) -> None:
    """A castle tower on the bank: coursed stone, an arrow slit, battlements."""
    top = ay + 3
    for y in range(top, base):
        half = 7
        for x in range(ax - half, ax + half + 1):
            course = (y - top) // 5
            joint = (y - top) % 5 == 4 or (x + (course % 2) * 4) % 8 == 0
            color = 13 if joint else 10
            if x == ax - half:
                color = 2
            elif x == ax + half:
                color = 1
            sput(px, x, y, color)
    # Battlements stand proud of the wall, with gaps between the merlons.
    for x in range(ax - 9, ax + 10):
        sput(px, x, top, 1)
        sput(px, x, top - 1, 10)
        sput(px, x, top - 2, 2 if (x - ax + 9) % 6 < 4 else 10)
        if (x - ax + 9) % 6 < 4:
            sput(px, x, top - 3, 10)
            sput(px, x, top - 4, 2)
    for y in range(top + 10, min(base - 6, top + 20)):
        sput(px, ax, y, 1)
    sput(px, ax - 1, top + 14, 1)
    sput(px, ax + 1, top + 14, 1)
    for x in range(ax - 9, ax + 10):
        sput(px, x, base - 1, 1)


def draw_prop(px: list[int], kind: str, x: int, deck: int, theme: dict) -> None:
    """Lineside scenery standing on the bank whose rail line is at deck."""
    ground = deck - 2
    if kind == "signal":
        # Semaphore signal, arm raised: the line is clear.
        for y in range(ground - 22, ground):
            sput(px, x, y, 1)
        sput(px, x - 1, ground - 1, 1)
        sput(px, x + 1, ground - 1, 1)
        for i in range(7):
            sput(px, x + 1 + i, ground - 21 - i // 2, 9)
            sput(px, x + 1 + i, ground - 20 - i // 2, 1)
        sput(px, x + 6, ground - 24, 2)
        sput(px, x, ground - 23, 3)
    elif kind == "lamp":
        for y in range(ground - 14, ground):
            sput(px, x, y, 1)
        srect(px, x - 1, ground - 17, x + 2, ground - 14, 3)
        sput(px, x, ground - 18, 1)
    elif kind == "tree":
        for y in range(ground - 6, ground):
            sput(px, x, y, 13)
        for dy in range(-16, -5):
            half = 5 - abs(dy + 11) // 2
            for dx in range(-half, half + 1):
                lit = dx < 0 and dy < -9
                sput(px, x + dx, ground + dy, theme["ground_top"] if lit else theme["ground_mid"])
        sput(px, x - 2, ground - 13, 2)
    elif kind == "station":
        # Knapford: a platform, a brick station building, an awning and a clock.
        srect(px, x - 30, ground - 3, x + 30, ground, 10)
        for i in range(x - 30, x + 30, 4):
            sput(px, i, ground - 3, 2)
        srect(px, x - 20, ground - 19, x + 20, ground - 3, 9)
        for y in range(ground - 18, ground - 3, 3):
            for i in range(x - 20, x + 20):
                sput(px, i, y, 13)
        for i in range(-23, 24):
            sput(px, x + i, ground - 20, 1)
            sput(px, x + i, ground - 21, 13)
        for i in range(-12, 13):
            sput(px, x + i, ground - 22 - (12 - abs(i)) // 3, 13)
            sput(px, x + i, ground - 23 - (12 - abs(i)) // 3, 1)
        for wx in (x - 15, x - 4, x + 10):
            srect(px, wx, ground - 15, wx + 5, ground - 8, 1)
            srect(px, wx + 1, ground - 14, wx + 4, ground - 9, 15)
        srect(px, x + 3, ground - 11, x + 8, ground - 3, 13)
        for i in range(-26, 27):
            sput(px, x + i, ground - 12 + abs(i) // 13, 2 if i % 4 else 9)
        srect(px, x - 2, ground - 26, x + 3, ground - 22, 2)
        sput(px, x, ground - 25, 1)
        sput(px, x, ground - 24, 1)
        sput(px, x + 1, ground - 24, 1)
    elif kind == "shed":
        # Tidmouth: an engine shed with an arched doorway.
        srect(px, x - 22, ground - 18, x + 22, ground, 13)
        for y in range(ground - 17, ground, 4):
            for i in range(x - 22, x + 22):
                sput(px, i, y, 5)
        for i in range(-25, 26):
            sput(px, x + i, ground - 19 - (25 - abs(i)) // 4, 1)
            sput(px, x + i, ground - 18 - (25 - abs(i)) // 4, 10)
        for door in (x - 12, x + 4):
            srect(px, door, ground - 12, door + 9, ground, 1)
            srect(px, door + 1, ground - 11, door + 8, ground, 7)
            sput(px, door + 1, ground - 11, 1)
            sput(px, door + 7, ground - 11, 1)
            sput(px, door + 4, ground - 2, 10)


def make_bg_terrain() -> None:
    levels = parse_levels()
    for index, (level, (name, kinds, props)) in enumerate(zip(levels, TERRAIN)):
        theme = BG_THEMES[level["theme"]]
        px = fill(256, 256, 0)
        water = level["water"]
        seeds = level["seeds"]
        (start_x, start_y), (end_x, end_y) = seeds[0], seeds[1]
        global _terrain_floor
        _terrain_floor = water

        # Back to front: stacks behind the line, then banks and what stands on them.
        for kind, (ax, ay) in zip(kinds, seeds[2:]):
            if kind == "crag":
                draw_crag(px, ax, ay, theme)
        draw_bank(px, -1, start_x, start_y, water, theme)
        draw_bank(px, 1, end_x, end_y, water, theme)
        for kind, (ax, ay) in zip(kinds, seeds[2:]):
            if kind == "rock":
                draw_rock(px, ax, ay, water, theme)
            elif kind == "ledge":
                draw_ledge(px, ax, ay, theme)
            elif kind == "tower":
                draw_tower(px, ax, ay, (start_y if ax < 0 else end_y) - 5)
        for kind, x in props:
            draw_prop(px, kind, x, (start_y if x < 0 else end_y) - 3, theme)

        write_indexed_bmp(os.path.join(GFX, f"bg_terrain_{index}.bmp"), 256, 256, px)
        write_json(f"bg_terrain_{index}.json", {"type": "regular_bg"})
        print(f"  terrain {index}: {name}")


HATT_16 = """
................
....11111111....
...1222222221...
..1244444444421.
..1244444444421.
..1244444444421.
..1244444444421.
..122222222221..
...1777777771...
...1773337771...
...1773337771...
...1777777771...
...1111111111...
...1555555551...
...1555555551...
...1111111111...
"""

HATT_16_ANGRY = """
................
....11111111....
...1222222221...
..1244444444421.
..1299999999421.
..1244444444421.
..1299999999421.
..122222222221..
...1777777771...
...1773337771...
...1773337771...
...1777777771...
...1111111111...
...1555555551...
...1555555551...
...1111111111...
"""


def draw_water_band(px: list[int], w: int, y0: int, y1: int) -> None:
    for y in range(y0, y1):
        base = 7 if y % 2 == 0 else 8
        for x in range(w):
            c = base
            if (x + y) % 7 == 0:
                c = 8
            if (x + y) % 11 == 0:
                c = 2
            put(px, w, x, y, c)


def draw_cliff_mass(px: list[int], w: int, x0: int, x1: int, top: int, bottom: int) -> None:
    rect(px, w, x0, top + 5, x1, bottom, 13)
    rect(px, w, x0 + 1, top + 6, x1 - 1, bottom - 1, 12)
    # Grass cap with highlight edge
    rect(px, w, x0, top, x1, top + 6, 14)
    rect(px, w, x0, top, x1, top + 2, 11)
    hline(px, w, x0, x1, top + 5, 13)
    # Track bed
    hline(px, w, x0 + 1, x1 - 1, top + 10, 12)
    hline(px, w, x0 + 1, x1 - 1, top + 11, 1)
    # Cliff edge highlight (light from top-left)
    for y in range(top + 6, bottom):
        put(px, w, x0 + 1, y, 12)


def make_splash() -> None:
    """128×128 illustration — four 64×64 tiles. No baked title text."""
    W = H = 128
    px = fill(W, H, 15)

    sky_gradient(px, W, H, 0, 58)
    blit_sun(px, W, 102, 16, 8)
    blit_painted_cloud(px, W, 22, 18, 1)
    blit_painted_cloud(px, W, 56, 12, 1)
    blit_painted_cloud(px, W, 84, 22, 1)

    # Distant tree line
    for x in range(W):
        base_y = 50 + abs((x - 64) // 6)
        put(px, W, x, base_y, 14)
        put(px, W, x, base_y + 1, 11)

    draw_cliff_mass(px, W, 0, 46, 56, 88)
    draw_cliff_mass(px, W, 82, W, 56, 88)

    # Canyon air stays sky, not magenta
    rect(px, W, 46, 58, 82, 88, 8)
    for y in range(58, 72):
        hline(px, W, 46, 82, y, 15 if y < 64 else 8)

    draw_water_band(px, W, 88, H)

    # Broken bridge — perspective planks
    for i, bx in enumerate(range(48, 80, 6)):
        by = 66 + (i % 2)
        rect(px, W, bx, by, bx + 5, by + 3, 5)
        hline(px, W, bx, bx + 5, by, 12)
        put(px, W, bx, by + 2, 1)

    # Suspension hint
    for bx in (50, 74):
        for y in range(69, 82):
            if y % 3 != 0:
                put(px, W, bx, y, 5)

    hero = fill(32, 32)
    _name, kind, colors, lining, dome, number, funnel_x, wheels = ENGINE_ART[0]
    draw_engine(hero, 32, 0, kind, colors, lining, dome, number, funnel_x, wheels, 1)
    for hy in range(32):
        for hx in range(32):
            if hero[hy * 32 + hx]:
                put(px, W, 6 + hx, 31 + hy, hero[hy * 32 + hx])
    draw_hatt_figure(px, W, 90, 48, angry=False)

    tile = 64
    out: list[int] = []
    for fy in range(2):
        for fx in range(2):
            for y in range(tile):
                for x in range(tile):
                    out.append(px[(fy * tile + y) * W + (fx * tile + x)])

    write_indexed_bmp(os.path.join(GFX, "splash.bmp"), tile, tile * 4, out)


def make_logo() -> None:
    """64×32 title plate — clean typography on branded panel (two 64×16 frames stacked = 64×32 invalid, use 64×64)."""
    w = h = 64
    px = fill(w, h)
    panel_box(px, w, 2, 8, 62, 56)
    rect(px, w, 6, 12, 58, 52, 7)
    rect(px, w, 8, 14, 56, 50, 6)
    hline(px, w, 8, 56, 28, 9)
    center_text(px, w, "THOMAS'S", 18, 3, 1)
    center_text(px, w, "BRIDGE", 32, 2, 1)
    # Small track icon
    hline(px, w, 22, 42, 46, 1)
    hline(px, w, 22, 42, 47, 12)
    blit_map(px, w, 26, 38, """
....111.
...1221.
..128821
..128821
...1991.
....111.
""")
    write_indexed_bmp(os.path.join(GFX, "logo.bmp"), w, h, px)


def make_sky() -> None:
    w = h = 64
    px = fill(w, h)
    sky_gradient(px, w, h, 0, h)
    write_indexed_bmp(os.path.join(GFX, "sky.bmp"), w, h, px)


def make_bg_sky_theme(theme_id: int) -> None:
    """Full-screen sky layer for regular BG parallax (256×256)."""
    theme = BG_THEMES[theme_id]
    w = h = 256
    px = fill(w, h)
    sky_gradient(px, w, h, 0, h, theme["sky"])
    haze_y = 150 if theme_id == 0 else (148 if theme_id == 1 else 146)
    haze_color = theme["sky"][1] if theme_id != 2 else 10
    horizon_haze(px, w, haze_y, haze_y + 8, haze_color)
    if theme_id == 0:
        blit_sun(px, w, 210, 28, 10)
        blit_painted_cloud(px, w, 48, 36, 2)
        blit_painted_cloud(px, w, 120, 22, 2)
        blit_painted_cloud(px, w, 188, 44, 1)
    elif theme_id == 1:
        blit_painted_cloud(px, w, 40, 28, 1)
        blit_painted_cloud(px, w, 160, 40, 2)
    else:
        blit_painted_cloud(px, w, 72, 30, 1)
        blit_painted_cloud(px, w, 200, 24, 1)
    name = f"bg_sky_{theme_id}.bmp"
    write_indexed_bmp(os.path.join(GFX, name), w, h, px)
    if theme_id == 0:
        write_indexed_bmp(os.path.join(GFX, "bg_sky.bmp"), w, h, px)


def make_bg_sky() -> None:
    for theme_id in range(len(BG_THEMES)):
        make_bg_sky_theme(theme_id)


def make_cloud() -> None:
    w = h = 16
    px = fill(w, h)
    blit_ellipse(px, w, 8, 9, 6, 4, 2)
    blit_ellipse(px, w, 5, 10, 4, 3, 2)
    blit_ellipse(px, w, 11, 10, 4, 3, 2)
    blit_ellipse(px, w, 8, 11, 5, 2, 15)
    write_indexed_bmp(os.path.join(GFX, "cloud.bmp"), w, h, px)


def make_panel() -> None:
    w, fh = 64, 32
    px = fill(w, fh * 3)
    panel_box(px, w, 2, 2, 62, 30)
    hline(px, w, 6, 58, 14, 8)
    y0 = 32
    panel_box(px, w, 2, y0 + 2, 62, y0 + 30)
    y0 = 64
    rect(px, w, 2, y0 + 4, 62, y0 + 28, 3)
    rect(px, w, 4, y0 + 6, 60, y0 + 26, 2)
    rect(px, w, 4, y0 + 6, 60, y0 + 9, 3)
    hline(px, w, 4, 60, y0 + 26, 7)
    write_indexed_bmp(os.path.join(GFX, "panel.bmp"), w, fh * 3, px)


def make_hatt() -> None:
    w, fh = 32, 32
    px = fill(w, fh * 2)
    draw_hatt_figure(px, w, 0, 0, angry=False)
    draw_hatt_figure(px, w, 0, 32, angry=True)
    write_indexed_bmp(os.path.join(GFX, "hatt.bmp"), w, fh * 2, px)


def make_cursor() -> None:
    w = h = 16
    px = fill(w, h)
    blit_map(px, w, 2, 2, """
......222.......
.....2332.......
....23332.......
...233332.......
..2333332.......
.233333322222...
233333222aaa2...
33333222aaaa2...
333222aaaaaa2...
33222aaaaaaa2...
3222aaaaaaaa2...
222aaaaaaaa2....
..aaaaaaaa2.....
...aaaaaa2......
....aaaa2.......
.....aa2........
""")
    write_indexed_bmp(os.path.join(GFX, "cursor.bmp"), w, h, px)


def make_node() -> None:
    """16×16 riveted trestle cap — visible joint at build nodes."""
    w = h = 16
    px = fill(w, h)
    blit_map(px, w, 0, 0, """
......111111......
.....1aaaaaa1.....
....1a522225a1....
...1a52222225a1...
...1a52222225a1...
...1a52222225a1...
...1a52222225a1...
....1a522225a1....
.....1aaaaaa1.....
......111111......
.....1......1.....
....1..1111..1....
....1.1aaaa1.1....
....1.1aaaa1.1....
....1..1111..1....
.....1......1.....
""")
    write_indexed_bmp(os.path.join(GFX, "node.bmp"), w, h, px)


def make_anchor() -> None:
    """16×16 bank anchor plate — bolted embankment tie."""
    w = h = 16
    px = fill(w, h)
    blit_map(px, w, 0, 0, """
....11111111....
...1dddddddd1...
..1ddccccccdd1..
..1dc111111cd1..
..1dc1aaaa1cd1..
..1dc1aaaa1cd1..
..1dc111111cd1..
..1ddccccccdd1..
...1dddddddd1...
....11111111....
....1......1....
....1.1111.1....
....1.1aa1.1....
....1.1aa1.1....
....1.1111.1....
....1......1....
""")
    write_indexed_bmp(os.path.join(GFX, "anchor.bmp"), w, h, px)


def draw_timber_beam(px: list[int], w: int, y0: int, body: int, hi: int, lo: int, rail: int) -> None:
    """16×8 timber deck with metal rail cap (master-plan track link)."""
    rect(px, w, 0, y0 + 3, 16, y0 + 11, body)
    hline(px, w, 0, 16, y0 + 3, hi)
    hline(px, w, 0, 16, y0 + 4, rail)
    hline(px, w, 0, 16, y0 + 5, 2)
    hline(px, w, 0, 16, y0 + 10, lo)
    hline(px, w, 0, 16, y0 + 11, 1)
    put(px, w, 0, y0 + 3, 1)
    put(px, w, 15, y0 + 3, 1)
    for x in range(2, 15, 3):
        put(px, w, x, y0 + 7, lo)
        put(px, w, x + 1, y0 + 8, hi if x % 6 else body)
    for x in range(4, 13, 4):
        put(px, w, x, y0 + 6, 10)
        put(px, w, x, y0 + 9, 10)


def make_strut() -> None:
    w, h = 16, 128
    px = fill(w, h)
    track_frames = (
        (5, 12, 13, 10),
        (9, 4, 1, 10),
        (5, 12, 13, 10),
        (9, 3, 1, 10),
    )
    for frame, colors in enumerate(track_frames):
        y0 = frame * 16
        body, hi, lo, rail = colors
        draw_timber_beam(px, w, y0, body, hi, lo, rail)
    # Brace rod frames (4–5): darker metal rod with rivet ends.
    for frame, (body, hi) in enumerate(((10, 2), (1, 12))):
        y0 = (4 + frame) * 16
        rect(px, w, 5, y0 + 7, 11, y0 + 9, body)
        hline(px, w, 5, 11, y0 + 7, hi)
        hline(px, w, 5, 11, y0 + 9, 1)
        put(px, w, 4, y0 + 8, hi)
        put(px, w, 12, y0 + 8, hi)
        vline(px, w, 5, y0 + 7, y0 + 10, 1)
        vline(px, w, 10, y0 + 7, y0 + 10, 1)
    # Frame 6 — snap crack (jagged break at center).
    y0 = 6 * 16
    rect(px, w, 0, y0 + 6, 7, y0 + 10, 5)
    rect(px, w, 9, y0 + 6, 16, y0 + 10, 5)
    hline(px, w, 0, 7, y0 + 7, 12)
    hline(px, w, 9, 16, y0 + 7, 12)
    hline(px, w, 0, 7, y0 + 9, 1)
    hline(px, w, 9, 16, y0 + 9, 1)
    put(px, w, 0, y0 + 6, 1)
    put(px, w, 15, y0 + 6, 1)
    put(px, w, 7, y0 + 5, 4)
    put(px, w, 8, y0 + 6, 9)
    put(px, w, 7, y0 + 7, 4)
    put(px, w, 8, y0 + 8, 1)
    put(px, w, 7, y0 + 9, 4)
    # Frame 7 — cable: a thin wire slug, stretched along the link like the rod.
    y0 = 7 * 16
    hline(px, w, 5, 11, y0 + 8, 10)
    hline(px, w, 5, 11, y0 + 9, 1)
    put(px, w, 6, y0 + 8, 2)
    put(px, w, 9, y0 + 8, 2)
    write_indexed_bmp(os.path.join(GFX, "strut.bmp"), w, h, px)


def make_ghost() -> None:
    w, h = 8, 8
    px = fill(w, h)
    for y in range(h):
        for x in range(w):
            d = abs(x - 3) + abs(y - 3)
            if d == 3:
                put(px, w, x, y, 8)
    write_indexed_bmp(os.path.join(GFX, "ghost.bmp"), w, h, px)


def make_ring() -> None:
    w, h = 16, 16
    px = fill(w, h)
    for y in range(h):
        for x in range(w):
            d2 = (x - 7) * (x - 7) + (y - 7) * (y - 7)
            if 16 <= d2 <= 36:
                put(px, w, x, y, 3)
            elif 9 <= d2 <= 12:
                put(px, w, x, y, 3)
    write_indexed_bmp(os.path.join(GFX, "ring.bmp"), w, h, px)


def make_digit() -> None:
    w, h = 80, 8
    px = fill(w, h)
    digits = "0123456789"
    for i, ch in enumerate(digits):
        blit_text(px, w, ch, i * 8 + 1, 1, 2, 1)
    write_indexed_bmp(os.path.join(GFX, "digit.bmp"), w, h, px)


def make_toast() -> None:
    w, fh = 32, 8
    texts = ("DUP!", "WEAK", "DEL", "SPENT", "LONG!")
    px = fill(w, fh * len(texts))
    colors = (9, 3, 10, 9, 3)
    for i, text in enumerate(texts):
        y0 = i * fh
        panel_box(px, w, 1, y0 + 1, 31, y0 + 7)
        center_text(px, w, text, y0 + 1, colors[i], 1)
    write_indexed_bmp(os.path.join(GFX, "toast.bmp"), w, fh * len(texts), px)


def make_scenery_far() -> None:
    w, h = 64, 32
    px = fill(w, h)
    sky_gradient(px, w, h, 0, 18)
    for x in range(w):
        hill = 18 + abs((x - 32) // 5)
        for y in range(hill, h):
            put(px, w, x, y, 14 if y == hill else 11)
    write_indexed_bmp(os.path.join(GFX, "scenery_far.bmp"), w, h, px)


def make_bg_hills_theme(theme_id: int) -> None:
    """Far hills on regular BG — transparent sky, rolling hills at bottom."""
    theme = BG_THEMES[theme_id]
    w = h = 256
    px = fill(w, h, 0)
    hill_base = 168 if theme_id == 0 else (172 if theme_id == 1 else 176)
    far_base = hill_base - 18
    for x in range(w):
        far_wave = abs((x + 40) // 9) % 22
        far = far_base + far_wave
        for y in range(far, h):
            put(px, w, x, y, theme["hill_shadow"])
    for x in range(w):
        wave = abs((x - 128) // (6 if theme_id != 2 else 5)) % 28
        hill = hill_base + wave
        for y in range(hill, h):
            edge = y == hill
            put(px, w, x, y, theme["hill_shadow"] if edge else theme["hill_light"])
            if edge and theme_id == 1 and x % 17 == 0:
                put(px, w, x, y, 1)
    if theme_id == 0:
        for x in range(24, w, 40):
            crest = hill_base + abs((x - 128) // 6) % 28
            blit_tree_silhouette(px, w, x, crest, 13, 14)
    elif theme_id == 1:
        for x in range(32, w - 16, 56):
            crest = hill_base + abs((x - 128) // 6) % 28
            blit_rock_outcrop(px, w, x, crest, theme["hill_light"], theme["hill_shadow"])
    if theme_id == 2:
        for x in range(40, 220, 48):
            factory_h = hill_base - 24 - (x % 96) // 16
            rect(px, w, x, factory_h, x + 20, hill_base, 10)
            rect(px, w, x + 4, factory_h - 8, x + 10, factory_h, 1)
            vline(px, w, x + 16, factory_h - 14, factory_h - 8, 1)
            hline(px, w, x + 14, x + 18, factory_h - 14, 4)
    name = f"bg_hills_{theme_id}.bmp"
    write_indexed_bmp(os.path.join(GFX, name), w, h, px)
    if theme_id == 0:
        write_indexed_bmp(os.path.join(GFX, "bg_hills.bmp"), w, h, px)


def make_bg_hills() -> None:
    for theme_id in range(len(BG_THEMES)):
        make_bg_hills_theme(theme_id)


def draw_ground_strip_themed(px: list[int], w: int, x0: int, x1: int, y0: int, theme: dict,
                             meadow: bool = False) -> None:
    """16 px bank ground band for regular BG (transparent index 0 elsewhere)."""
    rect(px, w, x0, y0, x1, y0 + 4, theme["ground_top"])
    rect(px, w, x0, y0 + 3, x1, y0 + 5, theme["ground_mid"])
    rect(px, w, x0, y0 + 5, x1, y0 + 16, theme["ground_fill"])
    rect(px, w, x0, y0 + 5, x1, y0 + 7, theme["ground_hi"])
    for x in range(x0, x1, 4):
        shade = theme["ground_hi"] if x % 8 else theme["ground_fill"]
        put(px, w, x, y0 + 8, shade)
        put(px, w, x, y0 + 10, shade)
    if meadow:
        for x in range(x0 + 6, x1 - 2, 12):
            grass_tuft(px, w, x, y0 + 2, theme["ground_top"], theme["ground_mid"])
    hline(px, w, x0, x1, y0 + 15, 1)


def make_bg_ground_theme(theme_id: int) -> None:
    """Bank ground on regular BG — side strips, transparent center gap for water."""
    theme = BG_THEMES[theme_id]
    w = h = 256
    px = fill(w, h, 0)
    y0 = 240
    meadow = theme_id == 0
    draw_ground_strip_themed(px, w, 0, 104, y0, theme, meadow=meadow)
    draw_ground_strip_themed(px, w, 152, w, y0, theme, meadow=meadow)
    name = f"bg_ground_{theme_id}.bmp"
    write_indexed_bmp(os.path.join(GFX, name), w, h, px)
    if theme_id == 0:
        write_indexed_bmp(os.path.join(GFX, "bg_ground.bmp"), w, h, px)


def make_bg_ground() -> None:
    for theme_id in range(len(BG_THEMES)):
        make_bg_ground_theme(theme_id)


WATER_BAND_TOP = 200
WATER_BAND_BOTTOM = 248


def make_bg_water_theme(theme_id: int) -> None:
    """Animated river on regular BG — two 256 px maps stacked vertically.

    The band runs from WATER_BAND_TOP (the surface) to WATER_BAND_BOTTOM: bright
    broken ripples near the top, settling into darker water below.
    """
    theme = BG_THEMES[theme_id]
    w = frame_h = 256
    px = fill(w, frame_h * 2, 0)
    for frame in range(2):
        y_base = frame * frame_h
        for y in range(WATER_BAND_TOP, WATER_BAND_BOTTOM):
            depth = y - WATER_BAND_TOP
            for x in range(w):
                c = theme["water_dark"]
                ripple = (x // 4 + y // 2 + frame * 2) % 5
                if depth == 0:
                    c = theme["water_spark"] if (x + frame * 3) % 6 < 2 else theme["water_light"]
                elif depth < 14:
                    if (y + frame) % 2 == 0:
                        c = theme["water_light"]
                    if ripple == 0:
                        c = theme["water_light"]
                    elif ripple == 1 and (x + y + frame) % 9 == 0:
                        c = theme["water_spark"]
                elif ripple == 0 and (y + frame) % 3 == 0:
                    c = theme["water_light"]
                elif depth > 30 and (x + y) % 2 == 0 and ripple > 2:
                    c = 1 if theme["water_dark"] != 1 else theme["water_dark"]
                put(px, w, x, y_base + y, c)
    name = f"bg_water_{theme_id}.bmp"
    write_indexed_bmp(os.path.join(GFX, name), w, frame_h * 2, px)
    if theme_id == 0:
        write_indexed_bmp(os.path.join(GFX, "bg_water.bmp"), w, frame_h * 2, px)


def make_bg_water() -> None:
    for theme_id in range(len(BG_THEMES)):
        make_bg_water_theme(theme_id)


def make_ground() -> None:
    w, h = 32, 16
    px = fill(w, h)
    rect(px, w, 0, 0, 32, 4, 11)
    rect(px, w, 0, 3, 32, 5, 14)
    rect(px, w, 0, 5, 32, 16, 13)
    rect(px, w, 0, 5, 32, 7, 12)
    for x in range(0, 32, 4):
        shade = 12 if x % 8 else 13
        put(px, w, x, 8, shade)
        put(px, w, x, 10, shade)
    hline(px, w, 0, 32, 15, 1)
    write_indexed_bmp(os.path.join(GFX, "ground.bmp"), w, h, px)


def make_water() -> None:
    w, h = 32, 16
    px = fill(w, h)
    for frame in range(2):
        y0 = frame * 8
        draw_water_band(px, w, y0, y0 + 8)
    write_indexed_bmp(os.path.join(GFX, "water.bmp"), w, h, px)


def make_labels() -> None:
    # Indices 0-7: core HUD. 8-10: onboarding hints. 11-16: rare PG lore alternates.
    # 17-18: build tool indicators. 19-34: eight level intro dialog pairs.
    # 35-38: extra rare strings from docs/lore-leak.md phrase book.
    texts = (
        "BUILD",
        "TEST!",
        "DELAY",
        "GRAND",
        "NO GO",
        "PEEP!",
        "ASHES",
        "START",
        "A PIN",
        "WELD!",
        "PAUSE",
        "CHUFF",
        "TOOT!",
        "OOPS!",
        "WAIT!",
        "AGAIN",
        "NEW!",
        "TRACK",
        "ROD",
        "CREEK",
        "PROP!",
        "HILL!",
        "BANK!",
        "FELL!",
        "TRUSS",
        "YARD!",
        "WOOD",
        "GAP!",
        "STRUT",
        "GATE!",
        "ROCK",
        "KEEP!",
        "STAYS",
        "AWAY!",
        "HARD",
        "HARK!",
        "OH NO",
        "LOOP!",
        "THIN!",
        # 39-45: HUD tags. 46: a finished deck no engine can climb.
        "LOAD",
        "CABLE",
        "PAR",
        "MAX",
        "PEAK",
        "WOOD",
        "COST",
        "STEEP",
    )
    w, fh = 32, 8
    px = fill(w, fh * len(texts))
    colors = (
        2, 2, 2, 3, 2, 3, 2, 3, 2, 2, 2, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 3, 3, 2, 3,
        2, 3, 2, 3, 2, 3, 2, 3, 2, 3,
        3, 9, 3, 3,
        2, 2, 3, 9, 9, 2, 2, 9,
    )
    for i, text in enumerate(texts):
        # Five glyphs is all a 32 px label holds.
        assert len(text) <= 5, text
        y0 = i * fh
        rect(px, w, 0, y0, 32, y0 + fh, 7 if i < 8 else 13)
        rect(px, w, 1, y0 + 1, 31, y0 + fh - 1, 6 if i < 8 else 7)
        center_text(px, w, text, y0 + 1, colors[i], 1)
    write_indexed_bmp(os.path.join(GFX, "label.bmp"), w, fh * len(texts), px)


def make_banner() -> None:
    w, fh = 64, 32
    entries = (
        ("PICK TRACK", "", 7),
        ("HATT OK!", "USEFUL", 11),
        ("DELAY!", "CONFUSION", 9),
        ("HATT OK!", "PEEP PEEP", 11),
        ("DELAY!", "LISTEN", 9),
        ("PICK AN", "ENGINE", 7),
        ("CINDERS", "AND ASHES!", 9),
        ("BUST MY", "BUFFERS!", 9),
        ("BOTHER!", "FELL DOWN", 9),
        ("REALLY", "USEFUL!", 11),
        ("PAUSED", "", 7),
    )
    px = fill(w, fh * len(entries))
    for frame, (title, sub, bg) in enumerate(entries):
        y0 = frame * fh
        panel_box(px, w, 2, y0 + 1, 62, y0 + 31)
        if bg == 11:
            rect(px, w, 6, y0 + 8, 58, y0 + 26, bg)
        center_text(px, w, title, y0 + 10, 2 if bg != 9 else 3, 1)
        if sub:
            center_text(px, w, sub, y0 + 20, 3, 1)
    write_indexed_bmp(os.path.join(GFX, "banner.bmp"), w, fh * len(entries), px)


OPTIONS = ("RESUME", "RESTART", "LEVELS", "TITLE", "NEXT", "REBUILD", "RETRY")


def make_option() -> None:
    """Menu rows that stack 16 px apart: a 64×16 plate in the middle of a 64×32
    frame (the GBA has no 64×16 sprite). Two frames per option: idle, chosen.
    """
    w, fh = 64, 32
    px = fill(w, fh * len(OPTIONS) * 2)
    for i, name in enumerate(OPTIONS):
        for chosen in (0, 1):
            y0 = (i * 2 + chosen) * fh + 8
            rect(px, w, 1, y0 + 1, 63, y0 + 15, 1 if chosen else 7)
            rect(px, w, 2, y0 + 2, 62, y0 + 14, 3 if chosen else 2)
            if chosen:
                rect(px, w, 2, y0 + 12, 62, y0 + 14, 12)
            center_text(px, w, name, y0 + 5, 1, 2 if chosen else 15)
    write_indexed_bmp(os.path.join(GFX, "option.bmp"), w, fh * len(OPTIONS) * 2, px)


def make_version() -> None:
    """Title-screen build plate, read from the VERSION file at the repo root."""
    with open(os.path.join(ROOT, "VERSION"), encoding="utf-8") as f:
        version = f.read().strip()
    w, h = 64, 32
    px = fill(w, h)
    text = "VER " + version
    half = (len(text) * 6 + 5) // 2
    rect(px, w, 32 - half - 1, 11, 32 + half + 1, 21, 7)
    rect(px, w, 32 - half, 12, 32 + half, 20, 1)
    center_text(px, w, text, 14, 15, 1)
    write_indexed_bmp(os.path.join(GFX, "version.bmp"), w, h, px)


def make_stars() -> None:
    """Star rating, 32×16: frames for 0 to 3 stars earned out of three."""
    w, fh = 32, 16
    star = (
        "....#....",
        "....#....",
        "...###...",
        "#########",
        ".#######.",
        "..#####..",
        "..#####..",
        ".###.###.",
        ".#.....#.",
    )
    px = fill(w, fh * 4)
    for earned in range(4):
        y0 = earned * fh
        for slot in range(3):
            x0 = 1 + slot * 10
            lit = slot < earned
            # Dark outline first, then the body.
            for dy, row in enumerate(star):
                for dx, cell in enumerate(row):
                    if cell == "#":
                        for ox, oy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                            put(px, w, x0 + dx + ox, y0 + 3 + dy + oy, 1)
            for dy, row in enumerate(star):
                for dx, cell in enumerate(row):
                    if cell == "#":
                        put(px, w, x0 + dx, y0 + 3 + dy, 3 if lit else 10)
    write_indexed_bmp(os.path.join(GFX, "stars.bmp"), w, fh * 4, px)


def make_track_card() -> None:
    """Level-select cards, 64×32: track number, then the name in full.

    One-word names get their hint underneath; two-word names use both lines.
    """
    w, fh = 64, 32
    entries = (
        ("1", "KNAPFORD", "PROP!", False, 11),
        ("2", "GORDON'S", "HILL", True, 3),
        ("3", "CULDEE", "FELL", True, 9),
        ("4", "TIDMOUTH", "WOOD", False, 11),
        ("5", "WELLSWORTH", "STRUT", False, 3),
        ("6", "CROVAN'S", "GATE", True, 9),
        ("7", "CALLAN", "STAYS", False, 11),
        ("8", "VICARSTOWN", "AWAY!", False, 3),
    )
    px = fill(w, fh * len(entries))
    for i, (num, line1, line2, name_continues, accent) in enumerate(entries):
        y0 = i * fh
        panel_box(px, w, 2, y0 + 2, 62, y0 + 30)
        rect(px, w, 4, y0 + 4, 60, y0 + 10, accent)
        center_text(px, w, num, y0 + 5, 7, 13)
        center_text(px, w, line1, y0 + 12, 1, 13, 5 if len(line1) >= 10 else 6)
        if name_continues:
            center_text(px, w, line2, y0 + 21, 1, 13)
        else:
            center_text(px, w, line2, y0 + 22, 3, 1)
    write_indexed_bmp(os.path.join(GFX, "track_card.bmp"), w, fh * len(entries), px)


def make_menu() -> None:
    """Menu rows, 64×32, wide enough for every engine and pause option in full."""
    w, fh = 64, 32
    names = (
        "KNAPFORD",
        "GORDON",
        "VICARS",
        "RESUME",
        "RESTART",
        "LEVELS",
        "TITLE",
        "THOMAS",
        "PERCY",
        "EDWARD",
        "JAMES",
        "GORDON",
        "HENRY",
    )
    px = fill(w, fh * len(names))
    for i, name in enumerate(names):
        y0 = i * fh
        is_level = i < 3
        is_engine = 7 <= i <= 12
        outer = 13 if is_level else (11 if is_engine else 7)
        inner = 2 if is_level else 15
        accent = 11 if i != 1 else 3
        if is_engine:
            accent = (6, 11, 6, 9, 7, 14)[i - 7]
        rect(px, w, 1, y0 + 3, 63, y0 + 29, outer)
        rect(px, w, 3, y0 + 5, 61, y0 + 27, inner)
        if is_level or is_engine:
            rect(px, w, 3, y0 + 5, 61, y0 + 14, accent)
        if is_level:
            center_text(px, w, f"{i + 1}", y0 + 7, 7, 13)
        elif is_engine:
            center_text(px, w, "NO " + "162543"[i - 7], y0 + 7, 2, 13)
        else:
            center_text(px, w, ">", y0 + 7, 1, 13)
        dark_text = is_level or is_engine
        center_text(px, w, name, y0 + 18, 1 if dark_text else 7, 13 if dark_text else 1)
    write_indexed_bmp(os.path.join(GFX, "menu.bmp"), w, fh * len(names), px)


def write_json(name: str, extra: dict | None = None) -> None:
    data = {"type": "sprite", "bpp_mode": "bpp_4"}
    if extra:
        data.update(extra)
    with open(os.path.join(GFX, name), "w", encoding="utf-8") as f:
        json.dump(data, f, indent=4)
        f.write("\n")


def validate() -> int:
    """Validate committed graphics + Butano JSON sidecars (no regeneration)."""
    validator = os.path.join(ROOT, "tools", "validate_graphics.py")
    result = subprocess.run([sys.executable, validator], cwd=ROOT, check=False)
    if result.returncode != 0:
        return result.returncode

    json_names = [
        "splash.json",
        "logo.json",
        "sky.json",
        "bg_sky.json",
        "bg_hills.json",
        "bg_ground.json",
        "bg_water.json",
        "cloud.json",
        "panel.json",
        "hatt.json",
        "cursor.json",
        "node.json",
        "anchor.json",
        "ghost.json",
        "ring.json",
        "strut.json",
        "train.json",
        "car.json",
        "portrait.json",
        "scenery_far.json",
        "ground.json",
        "water.json",
        "digit.json",
        "toast.json",
        "label.json",
        "banner.json",
        "menu.json",
        "track_card.json",
    ]
    for theme_id in range(len(BG_THEMES)):
        suffix = f"_{theme_id}"
        for layer in ("bg_sky", "bg_hills", "bg_ground", "bg_water"):
            json_names.append(f"{layer}{suffix}.json")
    for level_index in range(len(TERRAIN)):
        json_names.append(f"bg_terrain_{level_index}.json")

    errors: list[str] = []
    for name in json_names:
        path = os.path.join(GFX, name)
        if not os.path.isfile(path):
            errors.append(f"missing {name}")
            continue
        try:
            with open(path, encoding="utf-8") as handle:
                json.load(handle)
        except json.JSONDecodeError as exc:
            errors.append(f"{name}: invalid JSON ({exc})")

    if errors:
        for err in errors:
            print(f"FAIL {err}", file=sys.stderr)
        return 1

    print("ok graphics assets")
    return 0


def generate_presentation() -> None:
    """Regenerate illustrated presentation assets without touching train/strut BMPs."""
    os.makedirs(GFX, exist_ok=True)
    make_splash()
    make_logo()
    make_sky()
    make_bg_sky()
    make_cloud()
    make_hatt()
    make_scenery_far()
    make_bg_hills()
    write_json("hatt.json", {"height": 32})
    print(f"presentation sprites written to {GFX}")


def generate() -> None:
    os.makedirs(GFX, exist_ok=True)
    make_splash()
    make_logo()
    make_sky()
    make_bg_sky()
    make_cloud()
    make_panel()
    make_hatt()
    make_cursor()
    make_node()
    make_anchor()
    make_ghost()
    make_ring()
    make_strut()
    make_train()
    make_car()
    make_portrait()
    make_scenery_far()
    make_bg_hills()
    make_bg_ground()
    make_bg_water()
    make_bg_terrain()
    make_ground()
    make_water()
    make_digit()
    make_toast()
    make_labels()
    make_banner()
    make_menu()
    make_option()
    make_version()
    make_stars()
    make_track_card()

    for name, extra in (
        ("splash.json", {"height": 64}),
        ("logo.json", None),
        ("sky.json", None),
        ("bg_sky.json", {"type": "regular_bg"}),
        ("bg_hills.json", {"type": "regular_bg"}),
        ("bg_ground.json", {"type": "regular_bg"}),
        ("bg_water.json", {"type": "regular_bg", "height": 256}),
        ("cloud.json", None),
        ("panel.json", {"height": 32}),
        ("hatt.json", {"height": 32}),
        ("cursor.json", None),
        ("node.json", None),
        ("anchor.json", None),
        ("ghost.json", None),
        ("ring.json", None),
        ("strut.json", {"height": 16}),
        ("train.json", {"height": 32}),
        ("car.json", {"height": 32}),
        ("portrait.json", {"height": 32}),
        ("scenery_far.json", None),
        ("ground.json", None),
        ("water.json", {"height": 8}),
        ("digit.json", {"width": 8}),
        ("toast.json", {"height": 8}),
        ("label.json", {"height": 8}),
        ("banner.json", {"height": 32}),
        ("menu.json", {"height": 32}),
        ("option.json", {"height": 32}),
        ("version.json", None),
        ("stars.json", {"height": 16}),
        ("track_card.json", {"height": 32}),
    ):
        write_json(name, extra)

    for theme_id, theme in enumerate(BG_THEMES):
        suffix = f"_{theme_id}"
        for layer in ("bg_sky", "bg_hills", "bg_ground"):
            write_json(f"{layer}{suffix}.json", {"type": "regular_bg"})
        write_json(f"bg_water{suffix}.json", {"type": "regular_bg", "height": 256})
        print(f"  theme {theme_id}: {theme['name']}")
    print(f"Thomas's Bridge sprites written to {GFX}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate or validate Thomas's Bridge graphics.")
    parser.add_argument(
        "--check",
        action="store_true",
        help="validate committed BMPs and JSON sidecars only (do not overwrite art)",
    )
    parser.add_argument(
        "--presentation",
        action="store_true",
        help="regenerate splash/sky/hills/hatt only (does not overwrite train or strut BMPs)",
    )
    args = parser.parse_args()

    if args.check:
        return validate()
    if args.presentation:
        generate_presentation()
        return validate()
    generate()
    return validate()


if __name__ == "__main__":
    raise SystemExit(main())
