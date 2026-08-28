#!/usr/bin/env python3
"""Thomas's Bridge — professional GBA pixel-art generator.

Art rules: top-left light, 1px outlines on characters/props, 3-tone ramps,
ordered dither for gradients, no scaled-font title on the splash illustration.
"""

from __future__ import annotations

import json
import os
import struct

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


def sky_gradient(px: list[int], w: int, h: int, y0: int, y1: int) -> None:
    """Ordered dither sky ramp."""
    for y in range(y0, y1):
        t = (y - y0) / max(1, y1 - y0 - 1)
        base = 15 if t < 0.45 else (8 if t < 0.75 else 7)
        alt = 8 if base == 15 else (7 if base == 8 else 7)
        for x in range(w):
            put(px, w, x, y, base if (x + y) % 3 else alt)


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
# Hand-tuned sprite templates (32×32 engine for splash / 16×16 for gameplay)
# ---------------------------------------------------------------------------

TRAIN_32 = """
................................
................................
...........111..................
..........1221..................
.........128821.................
........1288821.................
.......12888821.................
......128888821.................
.....1288666821.................
....128866666821................
...12886444466821...............
..1288644444466821..............
.128866666666666821.............
128866666666666666821111111.....
1288666662226666668216666611....
1288666222222266668216222211....
1288666221112266668216222211....
1288666221112266668216222211....
1288666222222266668216222211....
1288666662226666668216666611....
128866666666666666821111111.....
.1288666666666666821............
..128866666666666821............
...1288666666666821.............
....128866666666821.............
.....1288666666821..............
......12886666821...............
.......128888821................
........1288821.................
.........1299921................
..........11111.................
11111111111111111111111111111111
"""

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

HATT_16 = """
.....11111.....
....12221.....
...1244421.....
...1244421.....
...1244421.....
...1244421.....
...1244421.....
...1222221.....
....177771.....
...1777771.....
...1777771.....
...1773371.....
...1773371.....
...1777771.....
...1777771.....
...1111111.....
"""

HATT_16_ANGRY = """
.....11111.....
....12221.....
...1244421.....
...1244421.....
...1299921.....
...1244421.....
...1299991.....
...1222221.....
....177771.....
...1777771.....
...1777771.....
...1773371.....
...1773371.....
...1777771.....
...1777771.....
...1111111.....
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
    w = h = 8
    px = fill(w, h)
    blit_map(px, w, 0, 0, """
..1111..
.1aaaa1.
1a5225a1
1a5225a1
1a5225a1
.1aaaa1.
..1111..
........
""")
    write_indexed_bmp(os.path.join(GFX, "node.bmp"), w, h, px)


def make_anchor() -> None:
    w = h = 8
    px = fill(w, h)
    blit_map(px, w, 0, 0, """
.111111.
1dddddd1
1dcccd1.
1dc3cd1.
1dcccd1.
1dddddd1
.111111.
........
""")
    write_indexed_bmp(os.path.join(GFX, "anchor.bmp"), w, h, px)


def make_strut() -> None:
    w, h = 16, 64
    px = fill(w, h)
    frames = (
        (5, 12, 1),
        (9, 4, 1),
        (8, 2, 1),
        (9, 3, 1),
    )
    for frame, colors in enumerate(frames):
        y0 = frame * 16
        body, hi, lo = colors
        rect(px, w, 0, y0 + 6, 16, y0 + 10, body)
        hline(px, w, 0, 16, y0 + 7, hi)
        hline(px, w, 0, 16, y0 + 9, lo)
        put(px, w, 0, y0 + 6, 1)
        put(px, w, 15, y0 + 6, 1)
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
    w, h = 16, 32
    px = fill(w, h)
    blit_map(px, w, 0, 0, TRAIN_16)
    # chuff frame — smoke puffs
    blit_map(px, w, 0, 16, TRAIN_16)
    for sx, sy in ((5, 16), (6, 16), (7, 16), (4, 17), (8, 17)):
        put(px, w, sx, sy, 10)
    write_indexed_bmp(os.path.join(GFX, "train.bmp"), w, h, px)


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
    # Indices 0-10: core HUD. 11-15: rare PG lore alternates (see docs/lore-leak.md).
    texts = (
        "BUILD",
        "TEST!",
        "DELAY",
        "USEFUL",
        "NO GO",
        "PEEP!",
        "ASHES!",
        "START",
        "LV 1",
        "LV 2",
        "LV 3",
        "CHUFF!",
        "TOOT!",
        "OOPS!",
        "WAIT!",
        "AGAIN!",
    )
    w, fh = 32, 8
    px = fill(w, fh * len(texts))
    colors = (2, 2, 2, 3, 2, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 3)
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


def make_menu() -> None:
    w, fh = 32, 32
    names = ("KNAPFORD", "GORDON", "VICARS", "RESUME", "RESTART", "LEVELS", "TITLE")
    px = fill(w, fh * len(names))
    for i, name in enumerate(names):
        y0 = i * fh
        outer = 13 if i < 3 else 7
        inner = 2 if i < 3 else 15
        accent = 11 if i != 1 else 3
        rect(px, w, 1, y0 + 3, 31, y0 + 29, outer)
        rect(px, w, 3, y0 + 5, 29, y0 + 27, inner)
        if i < 3:
            rect(px, w, 3, y0 + 5, 29, y0 + 9, accent)
        label = f"{i + 1}" if i < 3 else ">"
        center_text(px, w, label, y0 + 7, 7 if i < 3 else 1, 13)
        center_text(px, w, name[:5], y0 + 17, 1 if i < 3 else 7, 13 if i < 3 else 1)
    write_indexed_bmp(os.path.join(GFX, "menu.bmp"), w, fh * len(names), px)


def write_json(name: str, extra: dict | None = None) -> None:
    data = {"type": "sprite", "bpp_mode": "bpp_4"}
    if extra:
        data.update(extra)
    with open(os.path.join(GFX, name), "w", encoding="utf-8") as f:
        json.dump(data, f, indent=4)
        f.write("\n")


def main() -> None:
    os.makedirs(GFX, exist_ok=True)
    make_splash()
    make_logo()
    make_sky()
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
    make_ground()
    make_water()
    make_digit()
    make_toast()
    make_labels()
    make_banner()
    make_menu()

    for name, extra in (
        ("splash.json", {"height": 64}),
        ("logo.json", None),
        ("sky.json", None),
        ("cloud.json", None),
        ("panel.json", {"height": 32}),
        ("hatt.json", {"height": 16}),
        ("cursor.json", None),
        ("node.json", None),
        ("anchor.json", None),
        ("ghost.json", None),
        ("ring.json", None),
        ("strut.json", {"height": 16}),
        ("train.json", {"height": 16}),
        ("ground.json", None),
        ("water.json", {"height": 8}),
        ("digit.json", {"width": 8}),
        ("toast.json", {"height": 8}),
        ("label.json", {"height": 8}),
        ("banner.json", {"height": 32}),
        ("menu.json", {"height": 32}),
    ):
        write_json(name, extra)
    print(f"Thomas's Bridge sprites written to {GFX}")


if __name__ == "__main__":
    main()
