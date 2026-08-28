# Building the ROM

## Output

| Artifact | Path |
|----------|------|
| GBA ROM | `thomas_bridge.gba` (repo root after `make`) |
| Canonical copy | `rom/thomas_bridge.gba` |
| Web staging | `web/public/rom/thomas_bridge.gba` |

ROM header: **THOMASBRIDGE** / code **THOM**

## Layout with Butano

Recommended directory layout:

```text
butano/
  butano/          ← engine (contains butano.mak)
  morris-bridge-gba/   ← this repository
```

The Makefile auto-detects `vendor/butano/butano` when present (CI / Docker), otherwise defaults to `../butano`.

Override manually:

```bash
make LIBBUTANO=/absolute/path/to/butano/butano
```

## Requirements

- [devkitARM](https://devkitpro.org/) — `DEVKITARM`, `DEVKITPRO` environment variables
- Python 3 (`python3` for grit asset pipeline)
- [Butano](https://github.com/GValiente/butano) cloned alongside or under `vendor/`

## Native build

```bash
git clone https://github.com/GValiente/butano.git
cd butano
git clone https://origin.cursor.com/git/hackmods/morris-bridge-gba.git morris-bridge-gba
cd morris-bridge-gba
make -j$(nproc)
```

## Docker build (no local toolchain)

```bash
./scripts/build_rom.sh
```

Uses image `sunbr0/gba-butano-base:4dd4b16`, clones Butano into `vendor/butano`, runs `make`, copies ROM to `rom/` and `web/public/rom/`.

## Makefile highlights

| Variable | Value | Notes |
|----------|-------|-------|
| `TARGET` | `thomas_bridge` | Output basename |
| `ROMTITLE` | `THOMASBRIDGE` | 12-char GBA title |
| `ROMCODE` | `THOM` | 4-char game code |
| `AUDIOBACKEND` | `maxmod` | Direct Sound music (MOD) + WAV SFX |
| `USERCXXFLAGS` | `-std=c++20` | C++20 required |

## Clean

```bash
make clean
```

Removes `build/` and generated asset headers.

## Committing the prebuilt ROM

The repo tracks **`rom/thomas_bridge.gba`** so friends can play without a toolchain. Regenerate and commit it whenever a change affects the shipped binary.

### When to push an updated ROM

Commit a fresh `rom/thomas_bridge.gba` after any **major refactor** or content change, including:

- Gameplay / physics / level logic (`src/`, `include/`)
- Graphics or audio assets (`graphics/`, `tools/gen_graphics.py`, `tools/gen_audio.py`)
- Butano Makefile flags, audio backend, or build pipeline
- Merging multiple feature branches into `main` (integration smoke)

Docs-only edits do **not** require a ROM bump.

### How to refresh and commit

```bash
./scripts/build_rom.sh
git add rom/thomas_bridge.gba
git commit -m "chore(rom): refresh prebuilt thomas_bridge.gba"
git push origin main
```

`build_rom.sh` runs `gen_graphics.py`, `gen_audio.py`, Docker `make`, copies to `rom/`, and syncs `web/public/rom/` for local playtest.

**Verify before commit:** `make -C tests` and, if web changed, `./scripts/sync_rom_to_web.sh && cd web && npm run test:e2e`.

### CI vs committed ROM

GitHub Actions builds a fresh ROM on every push (see [CI/CD](ci-cd.md)). The committed copy is for **clone-and-play** convenience; keep it in sync with `main` after refactors so [Getting started](getting-started.md) and EverDrive users get current behavior.

See also [Session knowledge — ROM policy](knowledge.md#prebuilt-rom-policy).
