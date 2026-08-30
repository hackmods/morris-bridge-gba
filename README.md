# Thomas's Bridge

```
╔══════════════════════════════════════════════════════════╗
║  T H O M A S ' S   B R I D G E          Game Boy Advance ║
║  build spans · test the train · dodge confusion & delay  ║
╚══════════════════════════════════════════════════════════╝
        .-~~-.
       / 6  6 \
      |  BLUE  |~~~ PEEP!
       \__==__/
   ~~~~~|======|~~~~~   creek
  ██████|======|██████  your deck
       (_)  (_)         chuff~
```

**Thomas's Bridge** — a high-resolution bridge workshop in the browser, plus an optional Game Boy Advance ROM.

Help a cheeky blue tank engine cross Sodor gaps while **Sir Topham Hatt** watches for confusion and delay. Original art — a playful Thomas homage, **not** a licensed product.

[![Release](https://github.com/hackmods/morris-bridge-gba/actions/workflows/release.yml/badge.svg)](https://github.com/hackmods/morris-bridge-gba/actions/workflows/release.yml)
[![ROM](https://img.shields.io/badge/ROM-thomas__bridge.gba-blue)](rom/thomas_bridge.gba)
[![Docs](https://img.shields.io/badge/docs-wiki-informational)](docs/README.md)

## Play now

| Method | Command / file |
|--------|----------------|
| **Browser (product)** | `cd web && npm install && npm run dev` → **http://127.0.0.1:4317** |
| **GBA ROM (optional)** | [`rom/thomas_bridge.gba`](rom/thomas_bridge.gba) or **http://127.0.0.1:4317/legacy.html** |
| **Docker** | `./scripts/sync_rom_to_web.sh && docker build -t morris-bridge-gba . && docker run --rm -p 8080:80 morris-bridge-gba` |

The workshop is drag-to-weld on a high-res canvas (road / timber / steel). The GBA ROM remains at `/legacy.html`.

## Features

- **Workshop** — Drag-to-weld road, timber, and steel; build-mode stress heat; cost vs par; eight crossings
- **GBA ROM** — Hold **A** and drag to chain-weld; R swaps timber / rod; HUD cost vs par (optional `/legacy.html`)
- **Eight tracks** — Knapford through Sodor Summit, with brace tools, par cost, and ★ ratings
- **Five engines** — Thomas, Percy, Edward, James, Gordon with unlock progression + SRAM stars
- **BG parallax** — sky, hills, bank ground, and animated water on regular backgrounds
- **Screen flow** — attract → level/engine select → build → test → result (ScreenManager + AudioDirector)
- **Original score** — Island Line, Yard, Workshop, Full Steam MOD loops + chuff-sync SFX
- **Optional PG lore** — rare HUD whispers and intro panels ([lore-leak.md](docs/lore-leak.md))
- **Test pyramid** — host playthroughs per level, Playwright E2E, CI ROM build ([ship-qa.md](docs/ship-qa.md))

## Documentation

Full wiki: [`docs/README.md`](docs/README.md)

| Topic | Link |
|-------|------|
| Master plan (Phases 1–7) | [docs/master-plan.md](docs/master-plan.md) |
| Build the ROM | [docs/building-rom.md](docs/building-rom.md) |
| Web playtest | [docs/web-playtest.md](docs/web-playtest.md) |
| Hardware / EverDrive | [docs/hardware.md](docs/hardware.md) |

## Repository

| Name | Value |
|------|--------|
| **Game title** | Thomas's Bridge |
| **ROM file** | `thomas_bridge.gba` (`THOMASBRIDGE` / `THOM01`) |
| **Git repo** | [`hackmods/morris-bridge-gba`](https://github.com/hackmods/morris-bridge-gba) (historical folder name) |
| **License** | zlib (same as Butano) |

Pre-built ROM (~347 KB) is committed for clone-and-play. Rebuild with `./scripts/build_rom.sh` after gameplay or asset changes.
