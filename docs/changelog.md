# Changelog & project history

## 2026-10-11 — Cable, stress colours, the purse

Reconciled with `main` (PR #73 / #74), keeping this branch's simulation and re-implementing the ideas from #73 that still made sense on top of it:

- **Cable:** a third material — cheap, long, strong in tension, slack in compression. Unlocks on Gordon's Hill; the par designs now hang with cable and prop with rod (pars: Hill 10, Crovan's Gate 20, Callan 15, Vicarstown 25)
- **Delete a beam:** B while pointing at a link removes just that link
- **Graded stress colours:** amber at half strength, red at four-fifths, in test mode and in the post-run rewind
- **LOAD / PEAK** readout during a run: worst link now and worst so far, in percent
- **The purse:** a bridge may cost at most 1.5× par; the HUD's right-hand number switches from PAR to MAX once you pass par
- Not carried over from #73: the 8×8 grid, unbreakable stone piers, its load-sharing stress model, and the runtime L+R+START cheat (use the `MORRIS_PRELOAD_REFERENCE` QA build)

## 2026-10-11 — Sodor, properly

- **Henry** joins as No. 3 (unlocks on Culdee Fell); engine select runs No. 1 to No. 6. Save format unchanged — Henry is a new bit on the end
- **Real trains:** Thomas pulls Annie and Clarabel, Gordon the express, Henry the Flying Kipper, Percy a truck; Edward, Henry, James and Gordon have tenders. Engines are redrawn with their wheel arrangements
- **Troublesome Trucks** jolt their load ±25 % as they bump along, and hop on screen
- **Gordon's Hill is a hill:** the far bank is 32 px higher and Edward banks every train from behind
- **Grades matter:** engines slow uphill and run away downhill, sag included
- **Campaign reordered:** Knapford, Gordon's Hill, Culdee Fell, Tidmouth, Wellsworth, Crovan's Gate, Callan, and Vicarstown — the bridge to the mainland — last. Invented names (Knapford Creek, Wellsworth Gap, Callan Canal, Sodor Summit) are gone; Callan's pylons are castle towers
- **Catchphrases:** CINDERS AND ASHES!, BUST MY BUFFERS!, BOTHER! and REALLY USEFUL! banners, each tied to how the run ended
- Unlock order changed: Henry on track 3, James on track 4

## 2026-10-11 — Art that matches the game

- **Terrain per level:** each track has its own ground layer generated from its anchors — banks ending at the bank anchors, rails on ballast, cliffs down to a river, and the rock, crags, ledges, pylons and footing the anchors stand on. Knapford gets its station, Tidmouth its shed, every line a signal
- **Engines redrawn:** cab, boiler, dome, funnel, number and smokebox face; Thomas is a blue No. 1 tank engine, Percy a green saddle tank, James red with gold lining. Portraits are the face in livery
- **Coaches:** drawn behind Edward, James and Gordon. The run is won when the last coach is off the bridge, and lost if the link under any part of the train goes
- **Tool tag:** TRACK / ROD beside the HUD panel, blinking on swap; build hints moved to the top of the screen
- Engine select shows the whole train's load; Hatt no longer stands on the portrait
- Level intro and track-card subtitles name what each track asks for (PROP!, HANG!, TRUSS…)

## 2026-10-10 — Real bridge physics

The test run is now a structural simulation instead of a per-span rating.

- **Solver:** fixed-point pin-jointed spring truss ([Physics](physics.md)). Joints have weight and fall, links carry axial force and snap in tension or compression, anchors are the only solid ground, the bridge settles under its own weight before the train leaves, and a snapped link brings its neighbours down with it
- **Removed:** `Tension = Load × distance / length`, the length rating, the joint-count "reinforcement" multiplier and the triangle bonus. Decks no longer float
- **Build rules:** timber is limited to the level's beam length, rod to 1.5×; the weld ghost turns red past the limit
- **Routing:** shortest climbable track route; grades steeper than 1 in 2 are not a route
- **Levels:** all eight rebuilt around different solid ground (rock, crags, nothing, ledges, pylons, a deep footing), each with a par design proven by host tests — including that removing any one link from it loses the train
- **Engines:** distinct speeds; coaches are trailing axle loads; each level is timetabled for an engine and a lighter one clears it for 1★
- **Rewind:** after a test, links that carried ≥ 75 % of their strength glow in build mode
- **Lore lines** are triggered by what the player did (par win, failure streak, collapse before departure, wrong engine) instead of a 4 % dice roll
- **Saves:** format unchanged, but best costs recorded against the old levels are stale

Presentation bugs found while checking the new ROM in the emulator, all present before this change:

- **Fail screen soft-lock:** dismissing DELAY! left the game on the fail screen for good; it now returns to the build
- **Hidden bridge:** every sprite drew behind the ground and hill layers, so a deck was only visible in the 48 px gap in the bank art. Scenery layers now share the back priority
- **Clipped links:** a link was one 16 px sprite stretched to length, which the GBA clips past 32 px. Links are now runs of segments ([`graph_view.cpp`](../src/graph_view.cpp)); rods stretch their 6 px slug into a line
- **Mirrored diagonals:** link rotation used screen-down y as counter-clockwise
- **Result text:** the win/fail line sat behind its dialog panel and was overwritten by the HUD on the frame the run ended
- Scenery (sky, hills, banks, water) now rises with the deck height of the level
- Collapse plays for 45 frames before the fail screen; every snap is heard
> The two entries below (2026-09-24) describe PR #73 and #74 as merged to `main`. #73's materials, £ purse, 8×8 grid and load-path stress model were superseded by the truss solver above when the branches were reconciled; its ideas for cable, beam delete, graded stress colours and a budget were re-implemented on top of it.

## 2026-09-24 — UI / playability handoff plan

- Lessons from vision mockups vs placeholder ROM: systems ≠ shippable feel/look
- New track doc: [ui-playability-plan.md](ui-playability-plan.md) (menus → feel → chrome → Knapford art)
- Linked from [docs/README.md](README.md), [session-goal.md](session-goal.md), [knowledge.md](knowledge.md)

## 2026-09-24 — Vision refactor: materials, truss stress, EDIT/SIM/RESULT

- **Materials:** Wood £10 / Iron £25 / Stone £50 / Cable £15 with level £ budgets
- **Physics:** Load-path truss stress (`F/yield`), 4-band heatmap, cascade collapse → DERAILED
- **EDIT controls:** 8×8 grid; A draw, B delete beam, L/R cycle material, START run
- **Docs:** [diagnosis-vision.md](diagnosis-vision.md), rewritten [physics.md](physics.md)
- Host `make -C tests` green (all level playthroughs); ROM binary refresh via CI

## 2026-08-30 — GBA is the product (not a web workshop)

- The whole point of this repository is a **Game Boy Advance** game
- Web remains the mGBA playtest of `rom/thomas_bridge.gba`
- Closed PR **#70**, which had put a high-res workshop at `/` and the ROM on `/legacy.html`
- Note in [AGENTS.md](../AGENTS.md)

## 2026-08-30 — Larger illustrated playtest stage

- Web playtest canvas stage is taller (up to 8× GBA pixels) with a thicker timber/brass bezel
- Copy names the GBA game as the product; hold-A chain-weld and timber/rod in the how-to

## 2026-08-30 — Build-mode stress preview

- Placed timber/rod spans glow hot while building if they cannot take the selected engine
- Weld ghost uses rod strength (not timber-only) when drawing a brace
- Stress palette uses GBA 5-bit colors (8-bit RGB crashed Butano when a span first went hot)

## 2026-08-30 — Timber vs rod materials

- Track welds are **timber** (cost 1); braces are **rod** (cost 2, 1.25× strength)
- 3★ pars updated so every reference still clears on budget
- Materials stay independent of engine type (load vs capacity)
- Clone-and-play ROM **354,740 bytes** (timber/rod costs + live cost HUD)

## 2026-08-30 — Build HUD cost vs par

- Build-mode HUD digits show live **cost** (nodes + timber + 2× rods) on the left and **par** on the right so ★ efficiency is visible while welding
- Test-mode HUD still shows node/strut counts; engine load stays on the weight digits

## 2026-08-30 — Illustrated GBA presentation + chain-weld

- Hold **A** and drag to chain-draw joints every 24 px (Poly Bridge-style weld); tap A still joins
- 32×32 Sir Topham Hatt (stern / furious) instead of the 16×16 color blob
- Banded illustrated sky with painted clouds; splash canyon no longer shows magenta holes
- Track/brace materials, 8-track campaign, and GBA ROM remain the product (web is the mGBA playtest shell)


## 2026-08-30 — Illustrated playtest chrome + readable track cards

- Web playtest uses an illustrated paper/sky shell around the GBA canvas (the game remains the ROM)
- Track cards pack long station names (VICARSTOWN, WELLSWORTH) so they fit the 64×32 sprite


## 2026-08-30 — Eight-track GBA campaign + SRAM v4

- Eight data-driven tracks (Knapford through Sodor Summit) with distinct gaps, deck heights, and par costs
- SRAM **v4** (32 bytes): 2-bit stars × 8, per-track best cost, intro flags; v1–v3 saves still migrate
- Gordon unlocks after every track is cleared; Percy / Edward / James still reward the first three clears
- Level-select **track cards** for all eight stations; intro panels for each first visit
- Host playthrough + 3★ reference for every shipped level
- Release ROM **349,092 bytes** (eight-track + weld HUD)


## 2026-08-30 — GBA weld pass (still a GBA game)

- Hold **A** and drag to weld a span from the selected joint; release to plant
- Build starts with the left bank selected so the first weld has a from-joint
- HUD reads HOLD A / A WELD / START instead of the old A NOD cycle
- Track path is queried before a test run (`has_track_path`)



Session knowledge captured from the morris-bridge-gba / **Thomas's Bridge** prototype build.

## 2026-08-29 — Delta hardware QA import guide

- `docs/ship-qa.md`: Delta iOS steps for ROM import, H3 save seed, H6 force-quit
- `scripts/export_delta_saves.sh` writes import-ready `.sav` files for Delta/mGBA

## 2026-08-30 — v1.0.0 ship release

- Published [GitHub Release v1.0.0](https://github.com/hackmods/morris-bridge-gba/releases/tag/v1.0.0) (354,740 bytes)
- Hardware QA sign-off: web-proxy waiver documented in `docs/ship-qa.md`

## 2026-08-29 — Hardware QA web proxy E2E (H1–H5)

- `hardware-qa-proxy.spec.ts` maps H1–H5 to mGBA-WASM checks (34 E2E total)
- H6 power-cycle remains hardware-only; soft-reset proxy in `sram-persist.spec.ts`

## 2026-08-29 — Master plan sync + hardware QA reference builds

- Mark Phases 1–6 complete in `docs/master-plan.md` (Phase 7 hardware pending)
- Add H2/H3 reference build steps to `docs/ship-qa.md` for Delta/EverDrive sign-off

## 2026-08-29 — Release ROM + walkthrough refresh after BG art

- ROM **340,892 bytes** (BG parallax art pass baked in)
- Regenerated walkthrough PNGs 02–09 with new sky/hills/water themes

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
