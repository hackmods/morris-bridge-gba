# Graphics and assets

## Sprite list

| File | Size | Role |
|------|------|------|
| `cursor.bmp` | 8×8 | Build-mode crosshair |
| `node.bmp` | 8×8 | Structural pin |
| `strut.bmp` | 16×1 (line) | Beam segment (scaled/rotated) |
| `train.bmp` | 32×32 × 6 engines × 2 frames | Side-view engines drawn from parts: cab, boiler, dome, funnel, number, smokebox face. Wheels sit on the bottom row |
| `car.bmp` | 32×32 × 7 | What engines pull, one frame per `engine::CarKind`: three tenders, Annie, Clarabel, express coach, Troublesome Truck |
| `bg_terrain_0..7.bmp` | 256×256 | One ground layer per level, generated from the anchors in `src/content/level.cpp` |
| `bg_water_0..2.bmp` | 256×512 (2 frames) | River: a 48 px band from the surface down |
| `portrait.bmp` | 32×32 × 6 | Engine select portraits: the face in the engine's livery, with its number |
| `anchor.bmp` | 16×16 | Bank anchor plate (riveted) |
| `ground.bmp` | — | Canyon bank tiles |
| `water.bmp` | — | Gap / water backdrop |
| `label.bmp` | 32×8 frames | HUD text tiles (BUILD, TEST, hints, etc.) |

Each BMP has a matching **`.json`** in [`graphics/`](../graphics/) with `"type": "sprite"` (or bg type where applicable).

## Level terrain

`make_bg_terrain()` in [`tools/gen_graphics.py`](../tools/gen_graphics.py) reads
every level's anchors, theme, deck height and river height straight from
[`src/content/level.cpp`](../src/content/level.cpp) and draws that level's
ground layer in screen space: banks that end at the bank anchors with rails on
top and a cliff down to the river, and whatever each extra anchor stands on
(`rock`, `crag`, `ledge`, `tower`, listed per level in `TERRAIN`). Generation
fails if `TERRAIN` and `level.cpp` disagree on how many anchors a level has.

**Moving an anchor means regenerating the art:** edit `level.cpp`, run
`python3 tools/gen_graphics.py`, rebuild.

The game shows the terrain layer unscrolled and without parallax, because the
banks have to stay under the anchors. Sky and hills still drift.

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

All art is generator output — `python3 tools/gen_graphics.py` reproduces every
BMP in `graphics/`. To regenerate after editing the generator or a level:

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
