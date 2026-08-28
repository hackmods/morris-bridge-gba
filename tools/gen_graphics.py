#!/usr/bin/env python3
"""Generate Thomas's Bridge sprites — high-quality GBA pixel art."""

from __future__ import annotations

import json
import os
import struct

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
GFX = os.path.join(ROOT, "graphics")

# GBA-friendly palette — clean Pokémon-style UI + Sodor colours
PALETTE = [
    (255, 0, 255),  # 0 transparent
    (40, 40, 48),  # 1 outline / hat black
    (248, 248, 240),  # 2 UI cream white
    (248, 216, 72),  # 3 gold highlight
    (200, 72, 56),  # 4 stress red / cheek
    (120, 80, 48),  # 5 wood strut
    (72, 136, 208),  # 6 Thomas blue
    (32, 64, 136),  # 7 UI border / water deep
    (120, 208, 232),  # 8 water shimmer / cursor
    (208, 48, 48),  # 9 red stripe / danger
    (136, 136, 144),  # 10 metal grey
    (96, 184, 80),  # 11 grass bright
    (184, 152, 104),  # 12 dirt tan
    (88, 56, 32),  # 13 cliff dark
    (56, 120, 56),  # 14 grass shadow
    (168, 208, 248),  # 15 sky light
]


def _pad_row(row: bytes) -> bytes:
    pad = (4 - (len(row) % 4)) % 4
    return row + (b"\x00" * pad)


def write_indexed_bmp(path: str, width: int, height: int, pixels: list[int]) -> None:
    assert len(pixels) == width * height
    palette = bytearray()
    for i in range(256):
        if i < len(PALETTE):
            r, g, b = PALETTE[i]
        else:
            r = g = b = 0
        palette += struct.pack("<BBBB", b, g, r, 0)

    rows = bytearray()
    for y in range(height - 1, -1, -1):
        row = bytes(pixels[y * width : (y + 1) * width])
        rows += _pad_row(row)

    info_size = 40
    pixel_offset = 14 + info_size + 256 * 4
    file_size = pixel_offset + len(rows)

    with open(path, "wb") as handle:
        handle.write(struct.pack("<2sIHHI", b"BM", file_size, 0, 0, pixel_offset))
        handle.write(
            struct.pack(
                "<IiiHHIIiiII",
                info_size, width, height, 1, 8, 0, len(rows), 0, 0, 256, 0,
            )
        )
        handle.write(palette)
        handle.write(rows)


def fill(w: int, h: int, color: int = 0) -> list[int]:
    return [color] * (w * h)


def put(px: list[int], w: int, x: int, y: int, color: int) -> None:
    if 0 <= x < w:
        h = len(px) // w
        if 0 <= y < h:
            px[y * w + x] = color


def rect(px: list[int], w: int, x0: int, y0: int, x1: int, y1: int, color: int) -> None:
    for y in range(y0, y1):
        for x in range(x0, x1):
            put(px, w, x, y, color)


def circle(px: list[int], w: int, cx: int, cy: int, radius: int, color: int) -> None:
    r2 = radius * radius
    for y in range(cy - radius, cy + radius + 1):
        for x in range(cx - radius, cx + radius + 1):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r2:
                put(px, w, x, y, color)


def hline(px: list[int], w: int, x0: int, x1: int, y: int, color: int) -> None:
    for x in range(x0, x1):
        put(px, w, x, y, color)


def vline(px: list[int], w: int, x: int, y0: int, y1: int, color: int) -> None:
    for y in range(y0, y1):
        put(px, w, x, y, color)


def panel_box(px: list[int], w: int, x0: int, y0: int, x1: int, y1: int) -> None:
    """Pokémon-style double border dialog box."""
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
    "Y": ["10001", "10001", "01010", "00100", "00100"],
    "&": ["01100", "10101", "01010", "10100", "01101"],
    "!": ["00100", "00100", "00100", "00000", "00100"],
    "-": ["00000", "00000", "11111", "00000", "00000"],
    "'": ["00100", "00100", "01000", "00000", "00000"],
    "1": ["00100", "01100", "00100", "00100", "01110"],
    "2": ["01110", "10001", "00010", "00100", "11111"],
    "3": ["11110", "00001", "01110", "00001", "11110"],
    " ": ["00000", "00000", "00000", "00000", "00000"],
}


def blit_text(px: list[int], w: int, text: str, ox: int, oy: int, color: int) -> None:
    x = ox
    for ch in text:
        glyph = FONT.get(ch, FONT[" "])
        for gy, row in enumerate(glyph):
            for gx, bit in enumerate(row):
                if bit == "1":
                    put(px, w, x + gx, oy + gy, color)
        x += 6


def center_text(px: list[int], w: int, text: str, y: int, color: int) -> None:
    tw = len(text) * 6 - 1
    blit_text(px, w, text, max(0, (w - tw) // 2), y, color)


def blit_text_2x(px: list[int], w: int, text: str, ox: int, oy: int, color: int, shadow: int = 1) -> None:
    """Double-size title lettering with outline shadow."""
    x = ox
    for ch in text:
        glyph = FONT.get(ch, FONT[" "])
        for gy, row in enumerate(glyph):
            for gx, bit in enumerate(row):
                if bit != "1":
                    continue
                for dy in (0, 1):
                    for dx in (0, 1):
                        put(px, w, x + gx * 2 + dx, oy + gy * 2 + dy, color)
                        put(px, w, x + gx * 2 + dx + 1, oy + gy * 2 + dy + 1, shadow)
        x += 12


def draw_train_hero(px: list[int], w: int, ox: int, oy: int) -> None:
    """Detailed 28x22 tank engine for splash art."""
    # Shadow on track
    hline(px, w, ox - 2, ox + 30, oy + 21, 1)
    # Wheels
    for wx in (ox + 6, ox + 18):
        circle(px, w, wx, oy + 19, 3, 1)
        circle(px, w, wx, oy + 19, 2, 10)
        circle(px, w, wx, oy + 19, 1, 9)
    # Boiler
    rect(px, w, ox + 2, oy + 8, ox + 22, oy + 18, 6)
    rect(px, w, ox + 3, oy + 9, ox + 21, oy + 17, 7)
    hline(px, w, ox + 3, ox + 21, oy + 13, 9)
    hline(px, w, ox + 3, ox + 21, oy + 14, 9)
    # Face
    circle(px, w, ox + 12, oy + 13, 5, 2)
    put(px, w, ox + 10, oy + 11, 1)
    put(px, w, ox + 14, oy + 11, 1)
    put(px, w, ox + 10, oy + 15, 1)
    put(px, w, ox + 11, oy + 16, 1)
    put(px, w, ox + 12, oy + 16, 1)
    put(px, w, ox + 13, oy + 16, 1)
    put(px, w, ox + 14, oy + 15, 1)
    # Cab
    rect(px, w, ox + 20, oy + 4, ox + 28, oy + 18, 7)
    rect(px, w, ox + 21, oy + 5, ox + 27, oy + 10, 3)
    rect(px, w, ox + 22, oy + 6, ox + 26, oy + 9, 15)
    # Funnel + smoke
    rect(px, w, ox + 6, oy + 3, ox + 10, oy + 8, 1)
    rect(px, w, ox + 5, oy + 1, ox + 11, oy + 4, 10)
    for sx, sy in ((7, 0), (8, 0), (9, 0), (6, 1), (10, 1)):
        put(px, w, ox + sx, oy + sy, 10)
    # Buffer
    rect(px, w, ox, oy + 16, ox + 3, oy + 20, 9)


def draw_hatt_hero(px: list[int], w: int, ox: int, oy: int) -> None:
    """Sir Topham Hatt watching from the cliff — splash size."""
    rect(px, w, ox + 4, oy, ox + 16, oy + 5, 1)
    rect(px, w, ox + 2, oy + 5, ox + 18, oy + 8, 1)
    hline(px, w, ox + 2, ox + 18, oy + 5, 10)
    circle(px, w, ox + 10, oy + 14, 7, 4)
    circle(px, w, ox + 10, oy + 14, 6, 2)
    put(px, w, ox + 7, oy + 12, 1)
    put(px, w, ox + 13, oy + 12, 1)
    hline(px, w, ox + 7, ox + 14, oy + 16, 1)
    rect(px, w, ox + 5, oy + 20, ox + 15, oy + 26, 7)
    put(px, w, ox + 10, oy + 20, 3)


def draw_cliff(px: list[int], w: int, x0: int, x1: int, top: int, bottom: int, flip: bool = False) -> None:
    """Grass-topped cliff mass."""
    rect(px, w, x0, top + 4, x1, bottom, 13)
    rect(px, w, x0, top, x1, top + 6, 11)
    rect(px, w, x0 + 1, top + 1, x1 - 1, top + 5, 14)
    for i, tx in enumerate(range(x0 + 2, x1 - 2, 5)):
        tree_y = top - 2 - (i % 2)
        circle(px, w, tx, tree_y, 2, 14)
        circle(px, w, tx, tree_y - 1, 1, 11)
    # Track bed
    hline(px, w, x0, x1, top + 8, 12)
    hline(px, w, x0, x1, top + 9, 1)


def make_splash() -> None:
    """128x128 hero illustration split into four 64x64 GBA tiles."""
    big_w = big_h = 128
    big = fill(big_w, big_h)

    for y in range(56):
        c = 15 if y < 20 else (8 if y < 40 else 6)
        hline(big, big_w, 0, big_w, y, c)

    circle(big, big_w, 104, 18, 8, 3)
    circle(big, big_w, 104, 18, 6, 2)

    for cx, cy in ((20, 16), (56, 10), (88, 22)):
        circle(big, big_w, cx, cy, 4, 2)
        circle(big, big_w, cx + 5, cy - 1, 5, 2)
        circle(big, big_w, cx + 3, cy + 1, 3, 15)

    for x in range(big_w):
        hill_y = 52 + abs((x - 64) // 8)
        put(big, big_w, x, hill_y, 14)
        put(big, big_w, x, hill_y + 1, 11)

    for y in range(88, big_h):
        for x in range(big_w):
            put(big, big_w, x, y, 7 if (x + y) % 4 else 8)

    draw_cliff(big, big_w, 0, 42, 58, 92)
    draw_cliff(big, big_w, 86, big_w, 58, 92)

    # Broken bridge over gap
    for bx in range(44, 84, 8):
        rect(big, big_w, bx, 66, bx + 6, 70, 5)
        hline(big, big_w, bx, bx + 6, 67, 12)
    # Rope / missing planks
    vline(big, big_w, 52, 70, 82, 5)
    vline(big, big_w, 76, 70, 84, 5)

    draw_train_hero(big, big_w, 6, 58)
    draw_hatt_hero(big, big_w, 96, 52)

    # Title lettering baked into artwork
    blit_text_2x(big, big_w, "THOMAS'S", 16, 96, 3, 1)
    blit_text_2x(big, big_w, "BRIDGE", 34, 114, 2, 1)

    # Split into 4 x 64x64 frames (vertical strip for Butano)
    tile = 64
    out = []
    for fy in range(2):
        for fx in range(2):
            for y in range(tile):
                for x in range(tile):
                    out.append(big[(fy * tile + y) * big_w + (fx * tile + x)])

    write_indexed_bmp(os.path.join(GFX, "splash.bmp"), tile, tile * 4, out)


def make_sky() -> None:
    """64x64 sky gradient tile."""
    w = h = 64
    px = fill(w, h)
    for y in range(h):
        t = y / (h - 1)
        color = 15 if t < 0.5 else (6 if t < 0.85 else 7)
        hline(px, w, 0, w, y, color)
    for cx, cy in ((12, 18), (44, 28), (28, 10)):
        put(px, w, cx, cy, 2)
        put(px, w, cx + 1, cy, 2)
        put(px, w, cx + 2, cy, 2)
        put(px, w, cx, cy + 1, 15)
        put(px, w, cx + 1, cy + 1, 15)
    write_indexed_bmp(os.path.join(GFX, "sky.bmp"), w, h, px)


def make_cloud() -> None:
    w = h = 16
    px = fill(w, h)
    for cx, cy, r in ((5, 8, 3), (9, 7, 4), (12, 9, 2)):
        circle(px, w, cx, cy, r, 2)
    for cx, cy, r in ((5, 8, 2), (9, 7, 3), (12, 9, 1)):
        circle(px, w, cx, cy, r, 15)
    write_indexed_bmp(os.path.join(GFX, "cloud.bmp"), w, h, px)


def make_panel() -> None:
    """64x32 UI panels: HUD bar, dialog, selection highlight."""
    w, frame_h = 64, 32
    h = frame_h * 3
    px = fill(w, h)

    panel_box(px, w, 2, 2, 62, 30)
    center_text(px, w, "HUD", 12, 7)

    y0 = 32
    panel_box(px, w, 2, y0 + 2, 62, y0 + 30)
    rect(px, w, 6, y0 + 8, 58, y0 + 26, 2)

    y0 = 64
    rect(px, w, 2, y0 + 4, 62, y0 + 28, 3)
    rect(px, w, 4, y0 + 6, 60, y0 + 26, 2)
    rect(px, w, 4, y0 + 6, 60, y0 + 8, 3)
    hline(px, w, 4, 60, y0 + 26, 7)

    write_indexed_bmp(os.path.join(GFX, "panel.bmp"), w, h, px)


def make_hatt() -> None:
    """Sir Topham Hatt — meme fat controller (original art). Two 16x16 frames: stern / furious."""
    w, h = 16, 32
    px = fill(w, h)
    for frame in range(2):
        y0 = frame * 16
        # Top hat
        rect(px, w, 4, y0 + 1, 12, y0 + 5, 1)
        rect(px, w, 3, y0 + 5, 13, y0 + 7, 1)
        hline(px, w, 3, 13, y0 + 5, 10)
        # Face
        circle(px, w, 8, y0 + 10, 5, 4 if frame else 2)
        circle(px, w, 8, y0 + 10, 4, 2)
        put(px, w, 6, y0 + 9, 1)
        put(px, w, 10, y0 + 9, 1)
        if frame == 0:
            hline(px, w, 6, 11, y0 + 12, 1)
        else:
            rect(px, w, 5, y0 + 11, 11, y0 + 13, 9)
        # Suit
        rect(px, w, 5, y0 + 13, 11, y0 + 15, 7)
        put(px, w, 8, y0 + 13, 3)
    write_indexed_bmp(os.path.join(GFX, "hatt.bmp"), w, h, px)


def make_cursor() -> None:
    """Wrench cursor — Pokémon-style with white outline."""
    w = h = 16
    px = fill(w, h)
    for dy in range(5):
        for dx in range(5):
            if dx + dy <= 4:
                put(px, w, 4 + dx, 10 - dy, 1)
    rect(px, w, 9, 4, 13, 8, 10)
    rect(px, w, 10, 5, 12, 7, 2)
    circle(px, w, 6, 6, 2, 3)
    circle(px, w, 6, 6, 1, 2)
    write_indexed_bmp(os.path.join(GFX, "cursor.bmp"), w, h, px)


def make_node() -> None:
    w = h = 8
    px = fill(w, h)
    circle(px, w, 4, 4, 3, 10)
    circle(px, w, 4, 4, 2, 2)
    put(px, w, 4, 4, 5)
    write_indexed_bmp(os.path.join(GFX, "node.bmp"), w, h, px)


def make_anchor() -> None:
    w = h = 8
    px = fill(w, h)
    rect(px, w, 1, 1, 7, 7, 13)
    rect(px, w, 2, 2, 6, 6, 12)
    rect(px, w, 2, 2, 6, 3, 11)
    rect(px, w, 3, 3, 5, 5, 5)
    put(px, w, 4, 4, 3)
    write_indexed_bmp(os.path.join(GFX, "anchor.bmp"), w, h, px)


def make_strut() -> None:
    w, h = 16, 32
    px = fill(w, h)
    for frame, (body, hi, hot) in enumerate(((5, 12, False), (9, 4, True))):
        y0 = frame * 16
        rect(px, w, 0, y0 + 5, 16, y0 + 11, body)
        hline(px, w, 0, 16, y0 + 6, hi)
        hline(px, w, 0, 16, y0 + 9, 1 if not hot else 4)
        put(px, w, 0, y0 + 5, 1)
        put(px, w, 15, y0 + 5, 1)
    write_indexed_bmp(os.path.join(GFX, "strut.bmp"), w, h, px)


def make_train() -> None:
    """Cheeky blue tank engine — 16x32 two-frame chuff."""
    w, h = 16, 32
    px = fill(w, h)
    for frame in range(2):
        y0 = frame * 16
        for wx in (4, 11):
            circle(px, w, wx, y0 + 13, 2, 1)
            circle(px, w, wx, y0 + 13, 1, 9)
        rect(px, w, 1, y0 + 6, 11, y0 + 12, 6)
        rect(px, w, 2, y0 + 7, 10, y0 + 11, 7)
        hline(px, w, 2, 10, y0 + 9, 9)
        hline(px, w, 2, 10, y0 + 10, 9)
        circle(px, w, 6, y0 + 9, 3, 2)
        put(px, w, 5, y0 + 8, 1)
        put(px, w, 7, y0 + 8, 1)
        if frame == 0:
            put(px, w, 5, y0 + 10, 1)
            put(px, w, 6, y0 + 11, 1)
            put(px, w, 7, y0 + 10, 1)
        else:
            rect(px, w, 5, y0 + 10, 8, y0 + 11, 1)
        rect(px, w, 10, y0 + 3, 15, y0 + 12, 7)
        rect(px, w, 11, y0 + 4, 14, y0 + 7, 3)
        rect(px, w, 3, y0 + 2, 6, y0 + 6, 1)
        rect(px, w, 2, y0 + 1, 7, y0 + 3, 10)
        rect(px, w, 0, y0 + 11, 2, y0 + 13, 9)
        if frame == 1:
            for sx, sy in ((4, 0), (5, 0), (3, 1), (6, 1)):
                put(px, w, sx, y0 + sy, 10)
    write_indexed_bmp(os.path.join(GFX, "train.bmp"), w, h, px)


def make_ground() -> None:
    w, h = 32, 16
    px = fill(w, h)
    rect(px, w, 0, 0, 32, 5, 11)
    rect(px, w, 0, 4, 32, 6, 14)
    rect(px, w, 0, 6, 32, 16, 13)
    rect(px, w, 0, 6, 32, 8, 12)
    for x in range(0, 32, 5):
        vline(px, w, x, 8, 14, 12)
    hline(px, w, 0, 32, 15, 1)
    write_indexed_bmp(os.path.join(GFX, "ground.bmp"), w, h, px)


def make_water() -> None:
    """32x16 two-frame ripple."""
    w, h = 32, 16
    px = fill(w, h)
    for frame in range(2):
        y0 = frame * 8
        rect(px, w, 0, y0, 32, y0 + 8, 7)
        for x in range(0, 32, 4):
            ox = (x + frame * 2) % 32
            put(px, w, ox, y0 + 2, 8)
            put(px, w, (ox + 1) % 32, y0 + 4, 6)
            put(px, w, (ox + 2) % 32, y0 + 6, 8)
    write_indexed_bmp(os.path.join(GFX, "water.bmp"), w, h, px)


def make_labels() -> None:
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
    )
    w, fh = 32, 8
    px = fill(w, fh * len(texts))
    colors = (7, 6, 9, 11, 15, 11, 9, 3, 3, 3, 3)
    for i, text in enumerate(texts):
        center_text(px, w, text, i * fh + 1, colors[i])
    write_indexed_bmp(os.path.join(GFX, "label.bmp"), w, fh * len(texts), px)


def make_banner() -> None:
    """64x32: title, level select, Hatt approval, confusion & delay."""
    w, fh = 64, 32
    px = fill(w, fh * 4)

    y0 = 0
    panel_box(px, w, 2, y0 + 1, 62, y0 + 31)
    rect(px, w, 4, y0 + 4, 60, y0 + 20, 6)
    hline(px, w, 4, 60, y0 + 12, 9)
    center_text(px, w, "THOMAS'S", y0 + 4, 3)
    center_text(px, w, "BRIDGE", y0 + 13, 2)
    center_text(px, w, "USEFUL!", y0 + 22, 3)

    y0 = 32
    panel_box(px, w, 2, y0 + 1, 62, y0 + 31)
    center_text(px, w, "PICK TRACK", y0 + 12, 7)

    y0 = 64
    panel_box(px, w, 2, y0 + 1, 62, y0 + 31)
    rect(px, w, 4, y0 + 6, 60, y0 + 26, 11)
    center_text(px, w, "HATT OK!", y0 + 12, 2)

    y0 = 96
    panel_box(px, w, 2, y0 + 1, 62, y0 + 31)
    rect(px, w, 4, y0 + 6, 60, y0 + 26, 9)
    center_text(px, w, "DELAY!", y0 + 12, 2)

    write_indexed_bmp(os.path.join(GFX, "banner.bmp"), w, fh * 4, px)


def make_menu() -> None:
    """32x32 menu rows — meme level names (valid GBA sprite size)."""
    w, fh = 32, 32
    rows = (
        "1-KNA",
        "2-GOR",
        "3-DEL",
        "> GO",
        "> RST",
        "> LVL",
        "> TOP",
    )
    full_names = (
        "KNAPFORD",
        "GORDON",
        "VICARS",
        "RESUME",
        "RESTART",
        "LEVELS",
        "TITLE",
    )
    px = fill(w, fh * len(rows))
    for i, (code, name) in enumerate(zip(rows, full_names)):
        y0 = i * fh
        bg = 13 if i < 3 else 7
        rect(px, w, 1, y0 + 4, 31, y0 + 28, bg)
        rect(px, w, 3, y0 + 6, 29, y0 + 26, 2 if i < 3 else 15)
        if i < 3:
            rect(px, w, 3, y0 + 6, 29, y0 + 10, 11 if i != 1 else 3)
        center_text(px, w, code, y0 + 10, 7)
        center_text(px, w, name[:5], y0 + 18, 1)
    write_indexed_bmp(os.path.join(GFX, "menu.bmp"), w, fh * len(rows), px)


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
    make_sky()
    make_cloud()
    make_panel()
    make_hatt()
    make_cursor()
    make_node()
    make_anchor()
    make_strut()
    make_train()
    make_ground()
    make_water()
    make_labels()
    make_banner()
    make_menu()

    write_json("splash.json", {"height": 64})
    write_json("sky.json")
    write_json("cloud.json")
    write_json("panel.json", {"height": 32})
    write_json("hatt.json", {"height": 16})
    write_json("cursor.json")
    write_json("node.json")
    write_json("anchor.json")
    write_json("strut.json", {"height": 16})
    write_json("train.json", {"height": 16})
    write_json("ground.json")
    write_json("water.json", {"height": 8})
    write_json("label.json", {"height": 8})
    write_json("banner.json", {"height": 32})
    write_json("menu.json", {"height": 32})
    print(f"Thomas's Bridge sprites written to {GFX}")


if __name__ == "__main__":
    main()
