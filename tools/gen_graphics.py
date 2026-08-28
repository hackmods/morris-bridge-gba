#!/usr/bin/env python3
"""Generate Morris Bridge sprites — Pokémon-style UI, meme Thomas / Sir Topham Hatt energy."""

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
    center_text(px, w, "MORRIS", y0 + 5, 3)
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
    print(f"Morris Bridge sprites written to {GFX}")


if __name__ == "__main__":
    main()
