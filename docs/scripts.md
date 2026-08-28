# Scripts reference

All scripts live in [`scripts/`](../scripts/) and are intended to be run from the **repository root**.

## `build_rom.sh`

Build `leo_bridge.gba` using Docker (no local devkitARM required).

```bash
./scripts/build_rom.sh
```

**Steps:**

1. Clone Butano into `vendor/butano` if missing
2. Run `make` inside `sunbr0/gba-butano-base:4dd4b16`
3. Copy `leo_bridge.gba` → `rom/leo_bridge.gba`
4. Run `sync_rom_to_web.sh`

**Override image:**

```bash
GBA_BUILD_IMAGE=my-custom-butano-image ./scripts/build_rom.sh
```

## `sync_rom_to_web.sh`

Copy canonical ROM into the web playtest public folder.

```bash
./scripts/sync_rom_to_web.sh
```

Source: `rom/leo_bridge.gba`  
Dest: `web/public/rom/leo_bridge.gba`

Required before `npm run dev`, `docker build`, or CI-parity local builds.

## `tools/gen_graphics.py`

Regenerate placeholder indexed BMP sprites in `graphics/`. Run after editing sprite logic or when art is missing.

```bash
python3 tools/gen_graphics.py
```

## `tools/gen_audio.py`

Regenerate original parody music (MOD) and SFX (WAV), plus 16-bit previews in `audio_preview/`.

```bash
python3 tools/gen_audio.py
python3 tools/gen_audio.py --check
```

| Goal | Commands |
|------|----------|
| New ROM + web test | `./scripts/build_rom.sh && cd web && npm run dev` |
| Docker smoke test | `./scripts/sync_rom_to_web.sh && docker build -t morris-bridge-gba:local .` |
| Physics only | `make -C tests` |
| Full native ROM | `make -j$(nproc)` (with devkitARM + Butano) |
