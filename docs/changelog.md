# Changelog & project history

Session knowledge captured from the morris-bridge-gba / **Thomas's Bridge** prototype build.

## 2026-08-29 — Level select flavor + engine idle chuff (Phases 5–6)

- Level select shows track subtitle (`FLAT!`, `HILL!`, `LONG!`) from intro data; rare whispers still override (~4%)
- Engine select plays periodic idle chuff synced to train sprite frames

## 2026-08-29 — Source layout: core / session / content (Phase 2)

- `src/core/` — `physics.cpp`, `bridge_system.cpp` (simulation)
- `src/session/` — `run_score.cpp` (scoring)
- `src/content/` — `level.cpp`, `engine.cpp` (data tables)
- GBA platform code remains under `src/` + `src/screens/`

## 2026-08-29 — Sky regular BG layer (Phase 4 start)

- Move sky parallax from 8 OAM sprites to a **256×256 regular BG** (`bg_sky.bmp`)
- `BackdropView` uses `bn::regular_bg_ptr` on BG priority 3; clouds/hills/ground/water remain OAM for now
- `validate_graphics.py` checks `bg_sky.bmp`; `gen_graphics.py` generates it

## 2026-08-29 — Screen extraction (Phase 2)

- Split monolithic `main.cpp` into `game.h` / `game.cpp` + `src/screens/*` (attract, select, playing, pause, result)
- `main.cpp` is now init + `Game::update()` loop only (~16 lines)
- Shared UI constants moved to `include/game_ui.h`
- `validate_graphics.py` wired into `make -C tests`; strut sheet height corrected to 96px (6 frames)

## 2026-08-29 — Engine v2 ship wave (PRs #14–#19)

Merged engine v2 architecture and Phase 1–3/6 foundations into `main`:

| Area | Highlights |
|------|------------|
| **Simulation** | LinkRole track vs brace, per-level tools, host playthrough per level |
| **Session** | RunScore ★ efficiency, SRAM v2/v3 (stars, intro_seen) |
| **Structure** | AudioDirector, BackdropView, GraphView, screen types, ScreenManager dispatch |
| **Lore** | First-visit level intros, rare whispers, all-clear PEEP epilogue |
| **CI** | Web E2E, Docker web build fix (`vite-env.d.ts`), full 7-check green |

Reference solutions: Knapford, Gordon's Hill, Vicarstown (3★ at par). `main.cpp` ~1770 lines (down from ~2090).

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
