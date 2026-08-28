# Changelog & project history

Session knowledge captured from the morris-bridge-gba / Ryan's Bridge prototype build.

## 2026-08-28 — Documentation wiki

- Added `docs/` wiki with full architecture, physics, deploy, and repo guides
- Repository named **`morris-bridge-gba`** on Origin

## 2026-08-28 — CapRover + Release pipeline

- Multi-stage `Dockerfile` (Vite build → nginx)
- `captain-definition` for CapRover
- `.github/workflows/release.yml` replaces `build-gba.yml`
- Pipeline: ROM → host tests → GHCR image → CapRover deploy

## 2026-08-28 — Web playtest page

- `web/` Vite app with `@thenick775/mgba-wasm`
- Keyboard controls + real-time command queue (TPP-inspired, no Twitch API)
- `scripts/sync_rom_to_web.sh`
- Fix: command input focus disables emulator key capture

## 2026-08-28 — Prebuilt ROM in repo

- `rom/leo_bridge.gba` committed (~95 KB)
- `scripts/build_rom.sh` (Docker + Butano)
- Verified on EverDrive GBA guidance documented

## 2026-08-28 — Initial prototype

- Butano C++20 GBA game: build mode + test mode
- Fixed-point physics (`bn::fixed` 20.12)
- `BridgeSystem`: 16 nodes, 32 struts, BFS path, tension snap
- Host unit tests with `tests/host_bn/` mock
- Placeholder 8×8 / 16×16 sprites via `tools/gen_graphics.py`

## Key technical decisions

| Decision | Rationale |
|----------|-----------|
| No floating-point on GBA | Requirement; matches Butano idioms |
| `approx_sqrt` / `isqrt64` | Avoid BIOS sqrt; shared with host tests |
| Weakness scales with span length | Simple difficulty knob without per-material types |
| mGBA-WASM for browser | Full GBA compatibility vs partial JS emulators |
| nginx not Vite in production | Static files + header control for COOP/COEP |
| Docker ROM build in CI | Reproducible; no devkitARM on GitHub runners |
| Audio backend `null` | Scope reduction for mechanics prototype |

## Version tags (recommended)

Use semver tags to trigger versioned Docker deploys:

```bash
git tag v0.1.0
git push origin v0.1.0
```

Deploys `ghcr.io/<owner>/<repo>:v0.1.0` to CapRover.
