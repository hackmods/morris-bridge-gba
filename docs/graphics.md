# Graphics and assets

## Sprite list

| File | Size | Role |
|------|------|------|
| `cursor.bmp` | 8×8 | Build-mode crosshair |
| `node.bmp` | 8×8 | Structural pin |
| `strut.bmp` | 16×1 (line) | Beam segment (scaled/rotated) |
| `train.bmp` | 32×32 × 5 engines × 2 frames | Side-view tank engines (Thomas, Percy, Edward, James, Gordon) |
| `portrait.bmp` | 32×32 × 5 | Engine select portraits with name plate |
| `anchor.bmp` | 16×16 | Bank anchor plate (riveted) |
| `ground.bmp` | — | Canyon bank tiles |
| `water.bmp` | — | Gap / water backdrop |
| `label.bmp` | 32×8 frames | HUD text tiles (BUILD, TEST, hints, etc.) |

Each BMP has a matching **`.json`** in [`graphics/`](../graphics/) with `"type": "sprite"` (or bg type where applicable).

## Palette rules

- **Color index 0** = transparent (magenta in generated placeholders)
- Indexed BMP, no compression
- Multiples of 8 pixels for sprite dimensions

## Asset workflow (CI validate-only)

Committed BMPs in `graphics/` are the source of truth. CI and `./scripts/build_rom.sh` **validate** assets; they do not regenerate them on every build.

```bash
# Validate dimensions, palette, and Butano JSON sidecars (same as CI)
python3 tools/gen_graphics.py --check
```

To regenerate placeholders after editing `tools/gen_graphics.py` templates locally:

```bash
python3 tools/gen_graphics.py          # overwrites BMPs in graphics/
python3 tools/gen_graphics.py --check  # confirm before commit
```

Or rebuild the ROM with a one-shot regen:

```bash
REGEN_ASSETS=1 ./scripts/build_rom.sh
```

**Aseprite drop-in:** export indexed BMPs (≤16 colors, index 0 transparent) at the sizes in the table above, keep JSON basenames aligned, then run `--check` and commit. Butano generates `bn::sprite_items::<name>` from the BMP basename.

See [Future expansion — Graphics upgrade strategy](future-expansion.md#5-graphics-upgrade-strategy-immersive-on-gba).

## Rendering struts in code

[`src/main.cpp`](../src/main.cpp):

1. Compute midpoint between two node positions
2. Compute angle via `bn::degrees_diamond_angle` (no `atan2` float)
3. `sprite.set_rotation(angle)`
4. `sprite.set_horizontal_scale(length / STRUT_SPRITE_WIDTH)`

## Affine mats

Makefile sets `USERFLAGS := -DBN_CFG_SPRITE_AFFINE_MATS_MAX_ITEMS=32` for rotation/scale headroom.
