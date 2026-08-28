# Scripts reference

All scripts live in [`scripts/`](../scripts/) and are intended to be run from the **repository root**.

## `build_rom.sh`

Build `thomas_bridge.gba` using Docker (no local devkitARM required).

```bash
./scripts/build_rom.sh
```

**Steps:**

1. Clone Butano into `vendor/butano` if missing
2. Run `python3 tools/gen_graphics.py` and `python3 tools/gen_audio.py`
3. Run `make` inside `sunbr0/gba-butano-base:4dd4b16`
4. Copy `thomas_bridge.gba` → `rom/thomas_bridge.gba`
5. Run `sync_rom_to_web.sh`

**After major refactors:** commit the refreshed `rom/thomas_bridge.gba` — see [Building the ROM — Committing the prebuilt ROM](building-rom.md#committing-the-prebuilt-rom).

**Override image:**

```bash
GBA_BUILD_IMAGE=my-custom-butano-image ./scripts/build_rom.sh
```

## `sync_rom_to_web.sh`

Copy canonical ROM into the web playtest public folder.

```bash
./scripts/sync_rom_to_web.sh
```

Source: `rom/thomas_bridge.gba`  
Dest: `web/public/rom/thomas_bridge.gba`

Required before `npm run dev`, `npm run test:e2e`, `docker build`, or CI-parity local builds.

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
| Web E2E | `./scripts/sync_rom_to_web.sh && cd web && npm run test:e2e` |
| Full native ROM | `make -j$(nproc)` (with devkitARM + Butano) |
