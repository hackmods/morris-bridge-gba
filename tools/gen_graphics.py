#!/usr/bin/env python3
"""Generate 8bpp indexed BMP sprites for the Morris Bridge Butano asset pipeline.

Color 0 is transparent (magenta). All images use the same 16-color palette
so they can share a 4bpp sprite palette on hardware.

Run from repo root: python3 tools/gen_graphics.py
"""

from __future__ import annotations

import json
import os
import struct

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
GFX = os.path.join(ROOT, "graphics")

# 15-bit-friendly RGB palette (index 0 = transparent)
PALETTE = [
    (255, 0, 255),  # 0 transparent
    (16, 12, 16),  # 1 near-black
    (240, 236, 224),  # 2 ivory / face
    (236, 196, 48),  # 3 node yellow
    (208, 104, 32),  # 4 anchor orange
    (132, 80, 44),  # 5 strut brown
    (56, 108, 188),  # 6 Thomas blue
    (28, 40, 84),  # 7 navy
    (72, 220, 220),  # 8 cursor cyan
    (204, 44, 40),  # 9 Thomas red stripe
    (108, 108, 116),  # 10 gray
    (56, 156, 72),  # 11 grass
    (196, 156, 100),  # 12 tan
    (88, 56, 36),  # 13 dark brown
    (32, 72, 48),  # 14 dark grass
    (248, 216, 72),  # 15 gold
]


def _pad_row(row: bytes) -> bytes:
    pad = (4 - (len(row) % 4)) % 4
    return row + (b"\x00" * pad)


def write_indexed_bmp(path: str, width: int, height: int, pixels: list[int]) -> None:
    """Write an uncompressed 8bpp bottom-up BMP. pixels is top-to-bottom."""
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
    header_size = 14 + info_size
    pixel_offset = header_size + 256 * 4
    file_size = pixel_offset + len(rows)

    with open(path, "wb") as handle:
        handle.write(struct.pack("<2sIHHI", b"BM", file_size, 0, 0, pixel_offset))
        handle.write(
            struct.pack(
                "<IiiHHIIiiII",
                info_size,
                width,
                height,
                1,
                8,
                0,
                len(rows),
                0,
                0,
                256,
                0,
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
            if (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r2:
                put(px, w, x, y, color)


def hline(px: list[int], w: int, x0: int, x1: int, y: int, color: int) -> None:
    for x in range(x0, x1):
        put(px, w, x, y, color)


# 5x5 pixel font caps for HUD and menus.
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


def blit_text(px: list[int], w: int, text: str, origin_x: int, origin_y: int, color: int) -> None:
    x = origin_x
    for ch in text:
        glyph = FONT.get(ch, FONT[" "])
        for gy, row in enumerate(glyph):
            for gx, bit in enumerate(row):
                if bit == "1":
                    put(px, w, x + gx, origin_y + gy, color)
        x += 6


def center_text(px: list[int], w: int, text: str, y: int, color: int) -> None:
    text_w = len(text) * 6 - 1
    x0 = max(0, (w - text_w) // 2)
    blit_text(px, w, text, x0, y, color)


def make_cursor() -> None:
    w = h = 16
    px = fill(w, h)
    for i in range(16):
        put(px, w, 7, i, 8)
        put(px, w, 8, i, 8)
        put(px, w, i, 7, 8)
        put(px, w, i, 8, 8)
    rect(px, w, 6, 6, 10, 10, 0)
    rect(px, w, 7, 7, 9, 9, 2)
    write_indexed_bmp(os.path.join(GFX, "cursor.bmp"), w, h, px)


def make_node() -> None:
    w = h = 8
    px = fill(w, h)
    circle(px, w, 3, 3, 3, 3)
    circle(px, w, 3, 3, 1, 15)
    write_indexed_bmp(os.path.join(GFX, "node.bmp"), w, h, px)


def make_anchor() -> None:
    w = h = 8
    px = fill(w, h)
    rect(px, w, 1, 1, 7, 7, 4)
    rect(px, w, 2, 2, 6, 6, 15)
    rect(px, w, 3, 3, 5, 5, 13)
    write_indexed_bmp(os.path.join(GFX, "anchor.bmp"), w, h, px)


def make_strut() -> None:
    """Two 16x16 frames: idle brown beam, then stressed red beam."""
    w = 16
    h = 32
    px = fill(w, h)
    for frame, colors in enumerate(((5, 12), (9, 4))):
        y0 = frame * 16
        body, highlight = colors
        rect(px, w, 0, y0 + 6, 16, y0 + 10, body)
        rect(px, w, 0, y0 + 7, 16, y0 + 8, highlight)
        put(px, w, 0, y0 + 6, 1)
        put(px, w, 15, y0 + 6, 1)
        put(px, w, 0, y0 + 9, 1)
        put(px, w, 15, y0 + 9, 1)
    write_indexed_bmp(os.path.join(GFX, "strut.bmp"), w, h, px)


def make_train() -> None:
    """Thomas-inspired tank engine (original pixel art, not licensed artwork).

    Two 16x16 frames: idle and chuff (smoke puff) for a little personality.
    """
    w = 16
    h = 32
    px = fill(w, h)

    for frame in range(2):
        y0 = frame * 16
        # Wheels — grey rim, red hub (cheeky Thomas nod)
        for wx in (4, 11):
            circle(px, w, wx, y0 + 13, 2, 1)
            circle(px, w, wx, y0 + 13, 1, 9)
        # Boiler — Thomas blue with red stripe
        rect(px, w, 1, y0 + 6, 11, y0 + 12, 6)
        rect(px, w, 2, y0 + 7, 10, y0 + 11, 7)
        hline(px, w, 2, 10, y0 + 9, 9)
        hline(px, w, 2, 10, y0 + 10, 9)
        # Face circle on smokebox door
        circle(px, w, 6, y0 + 9, 3, 2)
        put(px, w, 5, y0 + 8, 1)  # left eye
        put(px, w, 7, y0 + 8, 1)  # right eye
        put(px, w, 5, y0 + 10, 1)
        put(px, w, 6, y0 + 10, 1)
        put(px, w, 7, y0 + 10, 1)  # smile
        # Cabin
        rect(px, w, 10, y0 + 3, 15, y0 + 12, 7)
        rect(px, w, 11, y0 + 4, 14, y0 + 7, 15)
        # Funnel — black stack, grey rim
        rect(px, w, 3, y0 + 2, 6, y0 + 6, 1)
        rect(px, w, 2, y0 + 1, 7, y0 + 3, 10)
        # Red buffer beam
        rect(px, w, 0, y0 + 11, 2, y0 + 13, 9)
        # Smoke puff on chuff frame
        if frame == 1:
            put(px, w, 4, y0 + 0, 10)
            put(px, w, 5, y0 + 0, 10)
            put(px, w, 3, y0 + 1, 10)
            put(px, w, 6, y0 + 1, 10)
            put(px, w, 4, y0 + 1, 2)
            put(px, w, 5, y0 + 1, 2)

    write_indexed_bmp(os.path.join(GFX, "train.bmp"), w, h, px)


def make_ground() -> None:
    w, h = 32, 16
    px = fill(w, h)
    rect(px, w, 0, 0, 32, 4, 11)
    rect(px, w, 0, 3, 32, 4, 14)
    rect(px, w, 0, 4, 32, 16, 13)
    rect(px, w, 0, 4, 32, 6, 5)
    for x, y in ((3, 8), (9, 12), (18, 7), (24, 11), (28, 9)):
        put(px, w, x, y, 12)
        put(px, w, x + 1, y, 5)
    write_indexed_bmp(os.path.join(GFX, "ground.bmp"), w, h, px)


def make_water() -> None:
    w, h = 32, 8
    px = fill(w, h, 7)
    for x in range(0, 32, 4):
        put(px, w, x, 2, 6)
        put(px, w, x + 1, 3, 8)
        put(px, w, x + 2, 5, 6)
    write_indexed_bmp(os.path.join(GFX, "water.bmp"), w, h, px)


def make_labels() -> None:
    """32x8 frames for in-game HUD and status messages."""
    texts = (
        "BUILD",
        "TEST",
        "FAIL",
        "WIN!",
        "NO GO",
        "PEEP!",
        "ASHES!",
        "MENU",
        "LV 1",
        "LV 2",
        "LV 3",
    )
    w = 32
    frame_h = 8
    h = frame_h * len(texts)
    px = fill(w, h)
    colors = (2, 8, 9, 11, 15, 11, 9, 8, 15, 15, 15)
    for index, text in enumerate(texts):
        y0 = index * frame_h
        center_text(px, w, text, y0 + 1, colors[index])
    write_indexed_bmp(os.path.join(GFX, "label.bmp"), w, h, px)


def make_banner() -> None:
    """Title and menu banner frames (64x32 each).

    Frame 0: Title screen
    Frame 1: Level select header
    Frame 2: Victory splash
    Frame 3: AI credit splash
    """
    w = 64
    frame_h = 32
    frames = 4
    h = frame_h * frames
    px = fill(w, h)

    y0 = 0
    rect(px, w, 2, y0 + 2, 62, y0 + 30, 7)
    rect(px, w, 4, y0 + 4, 60, y0 + 28, 6)
    hline(px, w, 4, 60, y0 + 14, 9)
    center_text(px, w, "MORRIS", y0 + 6, 15)
    center_text(px, w, "BRIDGE", y0 + 14, 2)
    center_text(px, w, "USEFUL!", y0 + 22, 15)

    y0 = 32
    rect(px, w, 4, y0 + 4, 60, y0 + 28, 13)
    rect(px, w, 6, y0 + 6, 58, y0 + 26, 7)
    center_text(px, w, "LEVELS", y0 + 12, 2)

    y0 = 64
    rect(px, w, 4, y0 + 6, 60, y0 + 26, 11)
    center_text(px, w, "PEEP PEEP!", y0 + 12, 15)

    y0 = 96
    rect(px, w, 4, y0 + 8, 60, y0 + 24, 1)
    center_text(px, w, "AI BUILT", y0 + 12, 15)

    write_indexed_bmp(os.path.join(GFX, "banner.bmp"), w, h, px)


def make_menu() -> None:
    """Menu highlight rows (32x16 each) — pairs with label level names."""
    w = 32
    frame_h = 16
    rows = (
        "KNAPFORD",
        "GORDON",
        "VICARS",
        "RESUME",
        "RESTART",
        "LEVELS",
        "TITLE",
    )
    h = frame_h * len(rows)
    px = fill(w, h)
    for index, text in enumerate(rows):
        y0 = index * frame_h
        bg = 13 if index < 3 else 7
        rect(px, w, 0, y0 + 2, 32, y0 + 14, bg)
        # Left-align; names may clip slightly on 32px width.
        blit_text(px, w, text[:5], 2, y0 + 5, 2 if index != 1 else 15)
    write_indexed_bmp(os.path.join(GFX, "menu.bmp"), w, h, px)


def write_json(name: str, extra: dict | None = None) -> None:
    data = {"type": "sprite", "bpp_mode": "bpp_4"}
    if extra:
        data.update(extra)
    with open(os.path.join(GFX, name), "w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=4)
        handle.write("\n")


def main() -> None:
    os.makedirs(GFX, exist_ok=True)
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

    write_json("cursor.json")
    write_json("node.json")
    write_json("anchor.json")
    write_json("strut.json", {"height": 16})
    write_json("train.json", {"height": 16})
    write_json("ground.json")
    write_json("water.json")
    write_json("label.json", {"height": 8})
    write_json("banner.json", {"height": 32})
    write_json("menu.json", {"height": 16})
    print(f"Morris Bridge sprites written to {GFX}")


if __name__ == "__main__":
    main()
