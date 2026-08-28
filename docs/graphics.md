# Graphics and assets

## Sprite list

| File | Size | Role |
|------|------|------|
| `cursor.bmp` | 16×16 | Build-mode corner brackets |
| `ghost.bmp` | 8×8 | Placement preview node |
| `ring.bmp` | 16×16 | Selection pulse ring |
| `node.bmp` | 8×8 | Structural pin |
| `strut.bmp` | 16×16 ×4 frames | Beam (idle, stressed, preview ok/weak) |
| `train.bmp` | 16×16 | Tank engine |
| `anchor.bmp` | 8×8 | Bank anchor marker |
| `sky.bmp` | 32×16 | Canyon sky strip |
| `ground.bmp` | 32×16 | Canyon bank tiles |
| `water.bmp` | 32×8 ×2 frames | Animated gap / water |
| `panel.bmp` | 32×8 | HUD bar segment |
| `digit.bmp` | 8×8 ×10 | Node/strut counters |
| `label.bmp` | 32×8 ×7 | HUD status text |
| `toast.bmp` | 48×8 ×3 | Inline feedback (DUP, WEAK, DEL) |

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

## Affine mats and sort layers

Makefile sets:

- `BN_CFG_SPRITE_AFFINE_MATS_MAX_ITEMS=32` — headroom for rotated/scaled struts
- `BN_CFG_SPRITES_MAX_SORT_LAYERS=32` — headroom for z-order draw layers

Use as few unique `z_order` values as possible. See [Troubleshooting — sort layers](troubleshooting.md#no-more-sprite-sort-layers-available-butano-assert).
