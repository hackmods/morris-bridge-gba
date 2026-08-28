# Morris Bridge

**Morris Bridge** — a Game Boy Advance bridge-building game (Butano / C++20).

Morris pins nodes across Sodor-inspired canyons so a cheeky blue tank engine can cross — while Sir Topham Hatt watches for **confusion and delay**. Long spans snap under tension. Original pixel art and meme energy; not a licensed Thomas product.

## Play now

| Method | Command / file |
|--------|-----------------|
| **ROM** | [`rom/morris_bridge.gba`](rom/morris_bridge.gba) → mGBA or EverDrive **GBA** |
| **Browser** | `./scripts/sync_rom_to_web.sh && cd web && npm install && npm run dev` |
| **Docker** | `./scripts/sync_rom_to_web.sh && docker build -t morris-bridge-gba . && docker run --rm -p 8080:80 morris-bridge-gba` |

## Features

- **Title screen & level select** — three levels: Knapford Creek, Gordon's Hill, Vicarstown Gap
- **Pause menu** — resume, restart, level select, title (Select during build)
- **Sir Topham Hatt** — stern/furious reactions on win and fail
- **Meme HUD** — PEEP!, ASHES!, DELAY!, HATT OK!
- **Pokémon-style UI** — bordered panels, sky backdrop, animated water

## Documentation wiki

Full project knowledge base: **[`docs/README.md`](docs/README.md)**

| Topic | Doc |
|-------|-----|
| Quick start | [docs/getting-started.md](docs/getting-started.md) |
| Game design & controls | [docs/game-design.md](docs/game-design.md) |
| Physics (fixed-point) | [docs/physics.md](docs/physics.md) |
| Build GBA ROM | [docs/building-rom.md](docs/building-rom.md) |
| Web playtest | [docs/web-playtest.md](docs/web-playtest.md) |
| Deploy (CapRover) | [docs/deployment.md](docs/deployment.md) |
| CI/CD pipeline | [docs/ci-cd.md](docs/ci-cd.md) |
| Hardware / EverDrive | [docs/hardware.md](docs/hardware.md) |
| Git & remotes | [docs/repository.md](docs/repository.md) |
| Troubleshooting | [docs/troubleshooting.md](docs/troubleshooting.md) |

## Repository

- **Origin:** `https://origin.cursor.com/git/hackmods/morris-bridge-gba.git`
- **Branch:** `main`
- **License:** zlib (same as Butano)

## CI/CD

Push to `main` runs [`.github/workflows/release.yml`](.github/workflows/release.yml):

`build-rom` → `host-tests` → `Docker (GHCR)` → `CapRover deploy`

## Project layout

```text
include/          physics.h, bridge_system.h
src/              GBA game source + main loop
graphics/         BMP sprites + Butano JSON
tools/            gen_graphics.py — sprite generator
rom/              Prebuilt morris_bridge.gba
tests/            Host physics tests (no GBA toolchain)
web/              Browser playtest (mGBA-WASM)
deploy/           nginx + CapRover notes
docs/             Wiki / knowledge base
scripts/          build_rom.sh, sync_rom_to_web.sh
Dockerfile        Production web image
```
