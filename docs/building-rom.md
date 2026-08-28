# Building the ROM

## Output

| Artifact | Path |
|----------|------|
| GBA ROM | `leo_bridge.gba` (repo root after `make`) |
| Canonical copy | `rom/leo_bridge.gba` |
| Web staging | `web/public/rom/leo_bridge.gba` |

ROM header: **RYANS BRIDGE** / code **RYAN**

## Layout with Butano

Recommended directory layout:

```text
butano/
  butano/          ← engine (contains butano.mak)
  leo_bridge/      ← this repository (clone morris-bridge-gba here)
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
git clone https://origin.cursor.com/git/hackmods/morris-bridge-gba.git leo_bridge
cd leo_bridge
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
| `TARGET` | `leo_bridge` | Output basename |
| `ROMTITLE` | `RYANS BRIDGE` | 12-char GBA title |
| `ROMCODE` | `RYAN` | 4-char game code |
| `AUDIOBACKEND` | `maxmod` | Direct Sound music (MOD) + WAV SFX |
| `USERCXXFLAGS` | `-std=c++20` | C++20 required |

## Clean

```bash
make clean
```

Removes `build/` and generated asset headers.
