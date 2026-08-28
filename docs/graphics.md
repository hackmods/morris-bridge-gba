# Graphics and assets

## Sprite list

| File | Size | Role |
|------|------|------|
| `cursor.bmp` | 8×8 | Build-mode crosshair |
| `node.bmp` | 8×8 | Structural pin |
| `strut.bmp` | 16×1 (line) | Beam segment (scaled/rotated) |
| `train.bmp` | 16×16 | Tank engine |
| `anchor.bmp` | 8×8 | Bank anchor marker |
| `ground.bmp` | — | Canyon bank tiles |
| `water.bmp` | — | Gap / water backdrop |
| `label.bmp` | 32×8 frames | HUD text tiles (BUILD, TEST, hints, etc.) |

Each BMP has a matching **`.json`** in [`graphics/`](../graphics/) with `"type": "sprite"` (or bg type where applicable).

## Palette rules

- **Color index 0** = transparent (magenta in generated placeholders)
- Indexed BMP, no compression
- Multiples of 8 pixels for sprite dimensions

## Regenerating placeholders

```bash
python3 tools/gen_graphics.py
```

This overwrites BMPs in `graphics/`. Replace with your own art but keep JSON filenames aligned — Butano generates `bn::sprite_items::<name>` from the BMP basename.

## Rendering struts in code

[`src/main.cpp`](../src/main.cpp):

1. Compute midpoint between two node positions
2. Compute angle via `bn::degrees_diamond_angle` (no `atan2` float)
3. `sprite.set_rotation(angle)`
4. `sprite.set_horizontal_scale(length / STRUT_SPRITE_WIDTH)`

## Affine mats

Makefile sets `USERFLAGS := -DBN_CFG_SPRITE_AFFINE_MATS_MAX_ITEMS=32` for rotation/scale headroom.
