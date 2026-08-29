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
    """Ordered dither sky ramp."""
    light, mid, dark = colors
    for y in range(y0, y1):
        t = (y - y0) / max(1, y1 - y0 - 1)
        base = light if t < 0.45 else (mid if t < 0.75 else dark)
        alt = mid if base == light else (dark if base == mid else dark)
        for x in range(w):
            put(px, w, x, y, base if (x + y) % 3 else alt)


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
    "'": ["00100", "00100", "01000", "00000", "00000"],
    "!": ["00100", "00100", "00100", "00000", "00100"],
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


def blit_text(px: list[int], w: int, text: str, ox: int, oy: int, color: int, shadow: int = 1) -> None:
    x = ox
    for ch in text:
        glyph = FONT.get(ch, FONT[" "])
        for gy, row in enumerate(glyph):
            for gx, bit in enumerate(row):
                if bit == "1":
                    put(px, w, x + gx + 1, oy + gy + 1, shadow)
                    put(px, w, x + gx, oy + gy, color)
        x += 6


def center_text(px: list[int], w: int, text: str, y: int, color: int, shadow: int = 1) -> None:
    tw = len(text) * 6 - 1
    blit_text(px, w, text, max(0, (w - tw) // 2), y, color, shadow)


# ---------------------------------------------------------------------------
# Hand-tuned engine sprites — 32×32 side view, one silhouette per engine
# Legend: 1 outline  2 face white  3 gold  4 cheek  6 body  7 body shadow
#         8 steam  9 stripe  a wheel/buffer  c wood highlight
# ---------------------------------------------------------------------------

THOMAS_32 = """
................................
................................
...............888..............
..............1881..............
.............12221..............
............1266621.............
...........12666621.............
..........126666621.............
.........1266666621.............
........12666666621.............
.......126666666621.............
......1266666666621.............
.....12666666666621.............
....126666666666621.............
...126664444666666621...........
..12666442244666666621..........
.12666666666666666666621........
1266666666222666666666621.......
1266666222222266666666621.......
1266666221112266666666621.......
1266666224442266666666621.......
1266666221112266666666621.......
1266666222222266666666621.......
1266666666222666666666621.......
1266666666666666666666621.......
.1266666666666666666621.........
..1266666666666666621...........
...126666666666666621...........
....1266666666666621............
.....1aaaa6666666621............
......1aaaa66666621.............
11111111111111111111111111111111
"""

PERCY_32 = """
................................
................................
..................888...........
.................18881..........
................122221..........
...............12bbb21..........
..............12bbbb621.........
.............12bbbb6621.........
............12bbbb66621.........
...........12bbbb666621.........
..........12bbbb6666621.........
.........12bbbb66666621.........
........12bbbb666666621.........
.......12bb6644466666621........
......12bb664422446666621.......
.....12bbbbbbbbbbbbbbbb6621.....
....12bbbbbbbb622bbbbbbbb6621...
...12bbbbbbbb62222bbbbbbbb6621..
..12bbbbbbbb62211122bbbbbbbb6621
.12bbbbbbbb62244422bbbbbbbb6621.
12bbbbbbbb62211122bbbbbbbb6621..
12bbbbbbbb62222bbbbbbbb6621.....
12bbbbbbbb622bbbbbbbb6621.......
.12bbbbbbbbbbbbbbbb6621.........
..12bbbbbbbbbbbb6621............
...12bbbbbbbb6621...............
....12bbbb6621..................
.....12bb6621...................
......1aa6621...................
.......1aa621...................
........1111....................
11111111111111111111111111111111
"""

EDWARD_32 = """
................................
................................
.............888................
............18881...............
...........122221...............
..........12555621..............
.........1255556621.............
........12555566621.............
.......125555666621.............
......1255556666621.............
.....12555566666621.............
....125555666666621.............
...1255556666666621.............
..125555666666666621............
.12555566666666666621...........
12555566666666666666621.........
12555566662255566666621.........
12555566222222566666621.........
12555566211122566666621.........
12555566244422566666621.........
12555566211122566666621.........
12555566222222566666621.........
12555566662255566666621.........
12555566666666666666621.........
.12555566666666666621...........
..1255556666666666621...........
...125555666666666621...........
....1255556666666621............
.....12555566666621.............
......1aaaa66666621.............
.......1aaaa6666621.............
11111111111111111111111111111111
"""

JAMES_32 = """
................................
................................
..............888...............
.............18881..............
............1223321.............
...........129996321............
..........1299996321............
.........12999996321............
........129999996321............
.......1299999996321............
......12999999996321............
.....129999999996321............
....1299999999996321............
...12999444999996321............
..1299944224499996321...........
.129999999999999996321..........
129999999999999999996321........
129999996622999999996321........
129999962222229999996321........
129999962111229999996321........
129999962444229999996321........
129999962111229999996321........
129999962222229999996321........
129999996622999999996321........
129999999999999999996321........
.129999999999999996321..........
..129999999999996321............
...1299999999996321.............
....12999999996321..............
.....1aaaa99996321..............
......1aaaa9996321..............
11111111111111111111111111111111
"""

GORDON_32 = """
................................
................................
............888888..............
...........1888881..............
..........12222221..............
.........12aaaaaa21.............
........12aaaaaaa621............
.......12aaaaaaaa621............
......12aaaaaaaa6621............
.....12aaaaaaaa66621............
....12aaaaaaaa666621............
...12aaaaaaaa6666621............
..12aaaaaaaa66666621............
.12aaaaaaaa666666621............
12aaaaaaaa66666666621...........
12aaaaaaaa6666666666621.........
12aaaaaaaa66662266666621........
12aaaaaaa666222266666621........
12aaaaaaa6662112266666621.......
12aaaaaaa6662444266666621.......
12aaaaaaa6662112266666621.......
12aaaaaaa666222266666621........
12aaaaaaaa66662266666621........
12aaaaaaaa6666666666621.........
.12aaaaaaaa66666666621..........
..12aaaaaaaa666666621...........
...12aaaaaaaa6666621............
....12aaaaaaaa66621.............
.....12aaaaaa6621...............
......1aaaaaa6621...............
.......1aaaa6621................
11111111111111111111111111111111
"""

TRAIN_32 = THOMAS_32  # splash hero engine

TRAIN_16 = """
.....111.......
....1221.......
...128821......
..12886621.....
.128864421.....
12886666621....
12886222221....
12886211221....
12886222221....
12886666621111.
.1288666821661.
..128866821111.
...1288821......
....19991......
.....111.......
111111111111111
"""


def tint_train_template(template: str, body: str, stripe: str, shadow: str = "7") -> str:
    rows = []
    for line in template.strip("\n").split("\n"):
        row = ""
        for ch in line:
            if ch == "6":
                row += body
            elif ch == "9":
                row += stripe
            elif ch == "7":
                row += shadow
            elif ch == "b":
                row += body
            elif ch == "5":
                row += body
            else:
                row += ch
        rows.append(row)
    return "\n".join(rows)


def validate_template(name: str, template: str, width: int = 32) -> None:
    lines = [ln for ln in template.strip("\n").split("\n") if ln]
    for i, line in enumerate(lines):
        if len(line) != width:
            raise ValueError(f"{name} row {i}: expected width {width}, got {len(line)}")


ENGINE_ART = (
    # name, template, body, stripe, shadow, steam funnel column, portrait accent
    ("Thomas", THOMAS_32, "6", "9", "7", 15, 6),
    ("Percy", PERCY_32, "b", "9", "e", 14, 11),
    ("Edward", EDWARD_32, "5", "c", "d", 13, 5),
    ("James", JAMES_32, "9", "3", "4", 14, 9),
    ("Gordon", GORDON_32, "a", "2", "1", 12, 10),
)

ENGINE_WEIGHTS = (4, 3, 5, 6, 10)

for _name, _tpl, *_rest in ENGINE_ART:
    validate_template(_name, _tpl)


def crop_template(template: str, drop_bottom: int = 0, drop_top: int = 0) -> str:
    lines = template.strip("\n").split("\n")
    if drop_bottom:
        lines = lines[:-drop_bottom]
    if drop_top:
        lines = lines[drop_top:]
    return "\n".join(lines)


def draw_steam_puffs(px: list[int], w: int, y0: int, funnel_x: int) -> None:
    """Chuff frame — wispy steam above the funnel."""
    puffs = (
        (funnel_x - 2, 1, 8),
        (funnel_x, 0, 2),
        (funnel_x + 1, 0, 8),
        (funnel_x + 3, 1, 2),
        (funnel_x - 1, 2, 8),
        (funnel_x + 2, 2, 15),
        (funnel_x, 3, 8),
        (funnel_x + 4, 2, 8),
    )
    for x, y, color in puffs:
        put(px, w, x, y0 + y, color)


def draw_wheel_glint(px: list[int], w: int, y0: int, frame: int) -> None:
    """Drive wheels with rim highlight — frame 1 adds connecting rod."""
    wheel_x = (11, 15, 19, 23)
    for x in wheel_x:
        rect(px, w, x, y0 + 26, x + 3, y0 + 29, 10)
        put(px, w, x + 1, y0 + 27, 1)
        put(px, w, x + 2, y0 + 28, 2 if frame == 0 else 12)
        hline(px, w, x, x + 3, y0 + 26, 1)
    if frame == 1:
        hline(px, w, 12, 22, y0 + 27, 1)
        put(px, w, 17, y0 + 26, 10)


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
    px = fill(W, H)

    sky_gradient(px, W, H, 0, 58)

    # Sun glow
    for dy in range(-10, 11):
        for dx in range(-10, 11):
            d2 = dx * dx + dy * dy
            if d2 <= 100:
                c = 3 if d2 < 36 else (2 if d2 < 64 else 15)
                put(px, W, 102 + dx, 14 + dy, c)

    # Clouds — soft layered puffs
    for cx, cy, scale in ((18, 20, 1), (54, 12, 1), (82, 24, 1)):
        for dy in range(-4 * scale, 5 * scale):
            for dx in range(-6 * scale, 7 * scale):
                if (dx / (6 * scale)) ** 2 + (dy / (4 * scale)) ** 2 <= 1:
                    put(px, W, cx + dx, cy + dy, 2)
                if (dx / (5 * scale)) ** 2 + (dy / (3 * scale)) ** 2 <= 1:
                    put(px, W, cx + dx, cy + dy, 15)

    # Distant tree line
    for x in range(W):
        base_y = 50 + abs((x - 64) // 6)
        put(px, W, x, base_y, 14)
        put(px, W, x, base_y + 1, 11)

    draw_cliff_mass(px, W, 0, 46, 56, 88)
    draw_cliff_mass(px, W, 82, W, 56, 88)

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

    blit_map(px, W, 4, 52, TRAIN_32)
    blit_map(px, W, 92, 50, HATT_16)

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
    blit_map(px, w, 1, 4, """
....2222....
...222222...
..22222222..
.2222222222.
.2222112222.
..22111122..
...111111...
....1111....
""".replace("2", "2"))
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
    w, h = 16, 32
    px = fill(w, h)
    blit_map(px, w, 0, 0, HATT_16)
    blit_map(px, w, 0, 16, HATT_16_ANGRY)
    write_indexed_bmp(os.path.join(GFX, "hatt.bmp"), w, h, px)


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
    w, h = 16, 112
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
    texts = ("DUP!", "WEAK", "DEL")
    px = fill(w, fh * len(texts))
    colors = (9, 3, 10)
    for i, text in enumerate(texts):
        y0 = i * fh
        panel_box(px, w, 1, y0 + 1, 31, y0 + 7)
        center_text(px, w, text, y0 + 1, colors[i], 1)
    write_indexed_bmp(os.path.join(GFX, "toast.bmp"), w, fh * len(texts), px)


def make_train() -> None:
    w, fh = 32, 32
    px = fill(w, fh * 2 * len(ENGINE_ART))
    for i, (_name, template, body, stripe, shadow, funnel_x, _accent) in enumerate(ENGINE_ART):
        y_base = i * fh * 2
        for frame in range(2):
            y0 = y_base + frame * fh
            art = tint_train_template(template, body, stripe, shadow)
            blit_map(px, w, 0, y0, art)
            draw_wheel_glint(px, w, y0, frame)
            if frame == 1:
                draw_steam_puffs(px, w, y0, funnel_x)
    write_indexed_bmp(os.path.join(GFX, "train.bmp"), w, fh * 2 * len(ENGINE_ART), px)


def make_portrait() -> None:
    w, fh = 32, 32
    px = fill(w, fh * len(ENGINE_ART))
    for i, (name, template, body, stripe, shadow, _funnel_x, accent) in enumerate(ENGINE_ART):
        y0 = i * fh
        rect(px, w, 1, y0 + 1, 30, y0 + 30, 1)
        rect(px, w, 2, y0 + 2, 29, y0 + 29, accent)
        rect(px, w, 3, y0 + 3, 28, y0 + 28, 7)
        rect(px, w, 4, y0 + 4, 27, y0 + 27, 2 if accent != 10 else 15)
        hline(px, w, 4, 28, y0 + 5, accent)
        art = tint_train_template(crop_template(template, drop_bottom=2, drop_top=2), body, stripe, shadow)
        blit_map(px, w, 0, y0 + 4, art)
        center_text(px, w, name[:5].upper(), y0 + 24, 1, 2)
        weight = str(ENGINE_WEIGHTS[i])
        rect(px, w, 22, y0 + 22, 30, y0 + 30, 1)
        rect(px, w, 23, y0 + 23, 29, y0 + 29, 3)
        blit_text(px, w, weight, 24, y0 + 24, 1, 7)
    write_indexed_bmp(os.path.join(GFX, "portrait.bmp"), w, fh * len(ENGINE_ART), px)


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
    for x in range(w):
        hill = hill_base + abs((x - 128) // (6 if theme_id != 2 else 5)) % 28
        for y in range(hill, h):
            put(px, w, x, y, theme["hill_shadow"] if y == hill else theme["hill_light"])
    if theme_id == 2:
        for x in range(40, 220, 48):
            factory_h = hill_base - 24 - (x % 96) // 16
            rect(px, w, x, factory_h, x + 20, hill_base, 10)
            rect(px, w, x + 4, factory_h - 8, x + 10, factory_h, 1)
    name = f"bg_hills_{theme_id}.bmp"
    write_indexed_bmp(os.path.join(GFX, name), w, h, px)
    if theme_id == 0:
        write_indexed_bmp(os.path.join(GFX, "bg_hills.bmp"), w, h, px)


def make_bg_hills() -> None:
    for theme_id in range(len(BG_THEMES)):
        make_bg_hills_theme(theme_id)


def draw_ground_strip_themed(px: list[int], w: int, x0: int, x1: int, y0: int, theme: dict) -> None:
    """16 px bank ground band for regular BG (transparent index 0 elsewhere)."""
    rect(px, w, x0, y0, x1, y0 + 4, theme["ground_top"])
    rect(px, w, x0, y0 + 3, x1, y0 + 5, theme["ground_mid"])
    rect(px, w, x0, y0 + 5, x1, y0 + 16, theme["ground_fill"])
    rect(px, w, x0, y0 + 5, x1, y0 + 7, theme["ground_hi"])
    for x in range(x0, x1, 4):
        shade = theme["ground_hi"] if x % 8 else theme["ground_fill"]
        put(px, w, x, y0 + 8, shade)
        put(px, w, x, y0 + 10, shade)
    hline(px, w, x0, x1, y0 + 15, 1)


def make_bg_ground_theme(theme_id: int) -> None:
    """Bank ground on regular BG — side strips, transparent center gap for water."""
    theme = BG_THEMES[theme_id]
    w = h = 256
    px = fill(w, h, 0)
    y0 = 240
    draw_ground_strip_themed(px, w, 0, 104, y0, theme)
    draw_ground_strip_themed(px, w, 152, w, y0, theme)
    name = f"bg_ground_{theme_id}.bmp"
    write_indexed_bmp(os.path.join(GFX, name), w, h, px)
    if theme_id == 0:
        write_indexed_bmp(os.path.join(GFX, "bg_ground.bmp"), w, h, px)


def make_bg_ground() -> None:
    for theme_id in range(len(BG_THEMES)):
        make_bg_ground_theme(theme_id)


def make_bg_water_theme(theme_id: int) -> None:
    """Animated water band on regular BG — two 256 px maps stacked vertically."""
    theme = BG_THEMES[theme_id]
    w = frame_h = 256
    px = fill(w, frame_h * 2, 0)
    for frame in range(2):
        y_base = frame * frame_h
        y0 = 232
        for y in range(y0, y0 + 16):
            base = theme["water_dark"] if (y + frame) % 2 == 0 else theme["water_light"]
            for x in range(w):
                c = base
                if (x + y + frame * 3) % 7 == 0:
                    c = theme["water_light"]
                if (x + y + frame * 5) % 11 == 0:
                    c = theme["water_spark"]
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
    # 17-18: build tool indicators. 19-24: level intro dialog pairs.
    # 25-28: extra rare strings from docs/lore-leak.md phrase book.
    texts = (
        "BUILD",
        "TEST!",
        "DELAY",
        "USEFUL",
        "NO GO",
        "PEEP!",
        "ASHES!",
        "START",
        "A NOD",
        "START",
        "PAUSE",
        "CHUFF!",
        "TOOT!",
        "OOPS!",
        "WAIT!",
        "AGAIN!",
        "NEW!",
        "TRACK",
        "ROD",
        "FLAT!",
        "SPANS",
        "HILL!",
        "BRACE",
        "LONG!",
        "RODS!",
        "LISTEN",
        "CONFUS",
        "LOOP!",
        "THIN!",
    )
    w, fh = 32, 8
    px = fill(w, fh * len(texts))
    colors = (
        2, 2, 2, 3, 2, 3, 2, 3, 2, 2, 2, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 3, 3, 2, 3,
        3, 9, 3, 3,
    )
    for i, text in enumerate(texts):
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
        ("PICK ENGIN", "LOAD", 7),
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


def make_track_card() -> None:
    """Level-select cards: full track name + subtitle, 64×32 (replaces tiny menu row)."""
    w, fh = 64, 32
    entries = (
        ("1", "KNAPFORD", "FLAT!", 11),
        ("2", "GORDON", "HILL!", 3),
        ("3", "VICARSTOWN", "LONG!", 9),
    )
    px = fill(w, fh * len(entries))
    for i, (num, title, sub, accent) in enumerate(entries):
        y0 = i * fh
        panel_box(px, w, 2, y0 + 2, 62, y0 + 30)
        rect(px, w, 4, y0 + 4, 60, y0 + 10, accent)
        center_text(px, w, num, y0 + 5, 7, 13)
        center_text(px, w, title, y0 + 12, 1, 13)
        center_text(px, w, sub, y0 + 22, 3, 1)
    write_indexed_bmp(os.path.join(GFX, "track_card.bmp"), w, fh * len(entries), px)


def make_menu() -> None:
    w, fh = 32, 32
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
    )
    px = fill(w, fh * len(names))
    for i, name in enumerate(names):
        y0 = i * fh
        is_level = i < 3
        is_engine = 7 <= i <= 11
        outer = 13 if is_level else (11 if is_engine else 7)
        inner = 2 if is_level else (15 if is_engine else 15)
        accent = 11 if i != 1 else 3
        if is_engine:
            accent = (6, 11, 5, 9, 10)[i - 7]
        rect(px, w, 1, y0 + 3, 31, y0 + 29, outer)
        rect(px, w, 3, y0 + 5, 29, y0 + 27, inner)
        if is_level or is_engine:
            rect(px, w, 3, y0 + 5, 29, y0 + 9, accent)
        if is_level:
            label = f"{i + 1}"
            center_text(px, w, label, y0 + 7, 7, 13)
        elif is_engine:
            center_text(px, w, f"E{i - 6}", y0 + 7, 1, 13)
        else:
            center_text(px, w, ">", y0 + 7, 1, 13)
        center_text(px, w, name[:5], y0 + 17, 1 if is_level or is_engine else 7, 13 if is_level or is_engine else 1)
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
    make_portrait()
    make_scenery_far()
    make_bg_hills()
    make_bg_ground()
    make_bg_water()
    make_ground()
    make_water()
    make_digit()
    make_toast()
    make_labels()
    make_banner()
    make_menu()
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
        ("hatt.json", {"height": 16}),
        ("cursor.json", None),
        ("node.json", None),
        ("anchor.json", None),
        ("ghost.json", None),
        ("ring.json", None),
        ("strut.json", {"height": 16}),
        ("train.json", {"height": 32}),
        ("portrait.json", {"height": 32}),
        ("scenery_far.json", None),
        ("ground.json", None),
        ("water.json", {"height": 8}),
        ("digit.json", {"width": 8}),
        ("toast.json", {"height": 8}),
        ("label.json", {"height": 8}),
        ("banner.json", {"height": 32}),
        ("menu.json", {"height": 32}),
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
    args = parser.parse_args()

    if not args.check:
        generate()
    return validate()


if __name__ == "__main__":
    raise SystemExit(main())
