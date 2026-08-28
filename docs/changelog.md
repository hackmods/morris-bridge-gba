# Changelog & project history

Session knowledge captured from the morris-bridge-gba / **Thomas's Bridge** prototype build.

## 2026-08-28 — Refresh prebuilt ROM + CI audio fix

- CI `build-rom` now runs `tools/gen_audio.py` before `make` (fixes missing `bn_music_items_*` headers)
- `scripts/build_rom.sh` runs `gen_audio.py`; documents when to commit `rom/thomas_bridge.gba`
- Refreshed `rom/thomas_bridge.gba` after full expansion merge (~264 KB; five engines, SRAM, parallax, audio)

## 2026-08-28 — Expansion merge + E2E + train art (session close)

Merged open PRs **#5–#10** into `main`; deleted remote `cursor/*` branches after integration.

| PR | Highlights |
|----|------------|
| **#8** | Five engines, unlock progression, SRAM save, three levels, 32×32 sprites, parallax, juice |
| **#6** | `tools/gen_audio.py`, parody BGM/SFX, `game_constants.h`, ground snap, joint/triangle strut reinforcement |
| **#5** | Onboarding hints (`A NOD`, `START`, `PAUSE`), Vitest layout tests, wider backdrop |
| **#7** | PG lore doc + rare HUD alternates (superseded by expansion history) |
| **#9** | Distinct per-engine 32×32 train templates in `tools/gen_graphics.py` |
| **#10** | Playwright E2E (12 tests), `window.__TEST__` hooks, CI `web-e2e` job |

**E2E lessons:** Vite must bind `127.0.0.1`; use canvas screenshots in headless Chromium (not frame counters); `./scripts/sync_rom_to_web.sh` before local E2E.

See [Session knowledge](knowledge.md) for pitfalls, backlog, and stretch goals.

## 2026-08-28 — Engine v2 master plan

- Added [master-plan.md](master-plan.md): GBA architecture, LinkRole/session layer, AV spec, lore hooks, phased ship checklist
- Added [session-goal.md](session-goal.md): copy-paste goal for full implementation session through gold master

## 2026-08-28 — Documentation wiki

- Added `docs/` wiki with full architecture, physics, deploy, and repo guides
- Repository named **`morris-bridge-gba`** on Origin

## 2026-08-28 — CapRover + Release pipeline

- Multi-stage `Dockerfile` (Vite build → nginx)
- `captain-definition` for CapRover
- `.github/workflows/release.yml` replaces `build-gba.yml`
- Pipeline: ROM → host tests → web tests → web E2E → Docker image → CapRover deploy

## 2026-08-28 — Web playtest page

- `web/` Vite app with `@thenick775/mgba-wasm`
- Keyboard controls + real-time command queue (TPP-inspired, no Twitch API)
- `scripts/sync_rom_to_web.sh`
- Fix: command input focus disables emulator key capture

## 2026-08-28 — Prebuilt ROM in repo

- `rom/thomas_bridge.gba` committed (~264 KB)
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
| Audio backend `maxmod` | Original parody BGM + steam SFX (not licensed TV audio) |
| Playwright E2E over Vitest-only | Boots real WASM emulator; catches ROM/render regressions |
| Vite `host: 127.0.0.1` | Playwright webServer health check fails on IPv6-only localhost |

## Version tags (recommended)

Use semver tags to trigger versioned Docker deploys:

```bash
git tag v0.1.0
git push origin v0.1.0
```

Deploys `ghcr.io/<owner>/<repo>:v0.1.0` to CapRover.
