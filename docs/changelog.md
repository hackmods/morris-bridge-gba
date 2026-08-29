# Changelog & project history

Session knowledge captured from the morris-bridge-gba / **Thomas's Bridge** prototype build.

## 2026-08-29 — BG parallax art pass (Phase 4)

- Sky: horizon haze band per theme (gold meadow, storm hill, industrial viaduct)
- Hills: tree silhouettes (Knapford), rock outcrops (Gordon's Hill), factory chimneys (Vicarstown)
- Ground: meadow grass tufts on Knapford banks
- Water: wider ripple bands with spark highlights

## 2026-08-29 — SRAM stars/clear persist E2E (H6 proxy)

- `sram-persist.spec.ts`: all_unlocked walkthrough save keeps stars + clear flags across soft reset (29 E2E)

## 2026-08-29 — Walkthrough artifact refresh (Phase 7)

- Regenerated all 9 walkthrough PNGs after latest art pass (Hatt, train wheels, portraits, struts)
- `readSaveProgress()` prefers mGBA FS `.sav` when available

## 2026-08-29 — SRAM persist E2E + engine portrait badges (Phase 4/7)

- `readSaveProgress()` test hook decodes save_data v3 header
- `sram-persist.spec.ts` — H6 proxy: intro_seen survives soft reset (28 Playwright tests)
- Engine select portraits show weight badge (4/3/5/6/10)

## 2026-08-29 — Hatt bust + train wheel art (Phase 4)

- Hatt: wider top hat, coat shoulders, stern/furious expressions (16×16 frames)
- Train: visible drive wheels + connecting rod on chuff frame for all engines
- Regenerated `hatt.bmp` + `train.bmp`; release ROM refresh follows CI build

## 2026-08-29 — Authored strut/node/anchor sprites (Phase 4)

- Track links: 16×8 timber deck with metal rail cap, grain, and rivets
- Brace rods: metal rod frames with end caps (distinct from track)
- Nodes: 16×16 riveted trestle cap; anchors: 16×16 bank bolt plate
- Committed regenerated BMPs; CI validates via `gen_graphics.py --check`

## 2026-08-29 — Graphics validate-only CI + M4 pause E2E (Phase 4/7)

- `gen_graphics.py --check` validates committed BMPs + Butano JSON sidecars (no CI overwrite)
- Release workflow and `build_rom.sh` use `--check` for graphics and audio
- M4 E2E: pause → level select and pause → title (`ship-qa.spec.ts`); 26 Playwright tests

## 2026-08-29 — Level select track cards + release ROM refresh

- Replaced broken level select (empty panel + tiny 5-char menu row) with 64×32 **track_card** sprites: KNAPFORD/FLAT!, GORDON/HILL!, VICARSTOWN/LONG!
- Added missing **F** glyph to procedural font (KNAPFORD was rendering as KNAP ORD)
- Release ROM refreshed to **338,236 bytes** (includes track_card assets)

## 2026-08-29 — Vicarstown web win E2E (Phase 7)

- `?walkthrough=vicarstown` loads `all_unlocked.sav` for level 3 + James
- `buildVicarstownReferenceBridge()` + ship-qa win test; 24 Playwright tests

## 2026-08-29 — Gordon Hill web win E2E (Phase 7)

- Gordon walkthrough save unlocks Edward (reference playthrough engine)
- `navigateToGordonBuildMode()` + `buildGordonHillReferenceBridge()` helpers
- `ship-qa.spec.ts` Gordon win test; 23 Playwright tests total

## 2026-08-29 — M2 web fail E2E (Phase 7)

- `buildOverlongKnapfordSpan()` helper — single START→END span (matches host snap test)
- `ship-qa.spec.ts` M2 fail test; 22 Playwright tests total

## 2026-08-29 — M1 web win E2E (Phase 7)

- `holdButton` / `holdButtons` test hooks for build macros in mGBA-WASM playtest
- `buildKnapfordReferenceBridge()` helper + `ship-qa.spec.ts` M1 full win test
- `docs/ship-qa.md` updated — M1 web E2E gate (21 Playwright tests)

## 2026-08-29 — mGBA walkthrough save load fix (Phase 7)

- Web emulator seeds Gordon's Hill unlock via `uploadSaveOrSaveState` before first `loadGame` (`?walkthrough=gordon`)
- E2E helpers preserve query strings (no longer strip `?walkthrough=gordon` on ready wait)
- `gordon-save.spec.ts` asserts save byte5=1 and Gordon track card after Down on level select

## 2026-08-29 — Gordon's Hill walkthrough save + capture (Phase 7)

- `tools/gen_playtest_save.py` writes mGBA SRAM saves for web playtest walkthroughs
- Web playtest injects `gordon_hill_walkthrough.sav` via `__TEST__.loadWalkthroughSave()`
- Walkthrough capture spec screenshots Gordon's Hill build + ROD/brace toggle

## 2026-08-29 — Per-theme BG tilesets (Phase 4)

- Three BG theme sets (`knapford`, `hill`, `viaduct`) for sky/hills/ground/water layers
- `BackdropView::apply_theme()` swaps regular BG items from `level::Def::bg_theme_id`
- Vicarstown hides OAM clouds for industrial sky; hill theme uses warmer palette + embankment browns

## 2026-08-29 — Engine idle motifs (Phase 5)

- `yard.mod` extended to 14 patterns with per-engine bell motifs (patterns 4–13)
- Engine select jumps to the selected engine's motif via `AudioDirector::play_yard_engine_motif`
- Level select / intro use `play_yard_menu()` (patterns 0–1)

## 2026-08-29 — Stress palette + snap crack frame (Phase 4)

- Hot struts swap to a red stress palette while pulsing under load
- New strut frame 6 shows a cracked beam for 12 frames before the fail screen
- `validate_graphics.py` expects `strut.bmp` at 16×112

## 2026-08-29 — Yard unlock leitmotif (Phase 5)

- `yard.mod` loops four patterns including unlock bell figures (patterns 2–3)
- New `unlock.wav` sting on engine/level unlock win (`NEW!` flash)
- Documented in `docs/audio.md`

## 2026-08-29 — GitHub + README ASCII art overhaul

- Root `README.md`: ASCII banner, CI badge, updated feature list and repo table
- Added `.github/PULL_REQUEST_TEMPLATE.md` and issue template for bugs
- Renamed stale **Ryan's Bridge** / `ryans-bridge` references in `deploy/caprover.md`
- Synced ROM size (~284 KB) in `docs/ship-qa.md`, `docs/hardware.md`, `docs/README.md`

## 2026-08-29 — Lore strings + ROM refresh (Phase 6)

- Expanded rare PG label tables (`LISTEN`, `CONFUS`, `LOOP!`, `THIN!`) from `docs/lore-leak.md`
- Refreshed `rom/thomas_bridge.gba` to 290,572 bytes (ground/water BG + labels)

## 2026-08-29 — Ground/water regular BG (Phase 4)

- Bank ground and animated water moved from OAM to `bg_ground.bmp` / `bg_water.bmp`

## 2026-08-29 — Phase 7 ship QA (M1–M5)

- Added `docs/ship-qa.md` — CI gates, M1–M5 evidence matrix, hardware sign-off
- New Playwright spec `web/e2e/ship-qa.spec.ts` (M3 NO GO, M4 pause, M5 theater, M1 partial)
- Updated `docs/hardware.md` ROM metadata; linked ship QA from `docs/testing.md`
- Web walkthrough recording: title → level select → build → theater mode

## 2026-08-29 — Release ROM refresh (Phase 7)

- Committed `rom/thomas_bridge.gba` (~276 KB) synced from CI after hills BG merge
- Web public ROM updated via `scripts/sync_rom_to_web.sh`

## 2026-08-29 — Hills regular BG layer (Phase 4)

- Far hills moved from 4 OAM sprites to `bg_hills.bmp` regular BG (priority 2)
- Per-level hill Y offset preserved; ground/water later moved to regular BG in PR #28

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
