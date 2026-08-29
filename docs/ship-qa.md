# Ship QA — Phase 7 verification

**Product:** Thomas's Bridge (`thomas_bridge.gba`)  
**ROM:** `THOMASBRIDGE` / `THOM01` · ~333 KB (340,892 bytes) as of 2026-08-29  
**Release branch:** `main`

This document records master-plan **M1–M5** screen-flow QA and related ship gates. Physical GBA hardware was not available in the Cloud Agent environment; **mGBA-WASM web playtest** and **host playthrough tests** provide equivalent coverage where noted.

---

## Automated gates (CI)

| Gate | Command / job | Status |
|------|----------------|--------|
| Host physics + session | `make -C tests` | Green on `main` |
| All-level playthroughs | `test_physics` Knapford / Gordon / Vicarstown 3★ | Green |
| Web unit tests | `cd web && npm test` | Green |
| Web E2E (all-level wins, M1–M2, M3–M5, Gordon save) | `npm run test:e2e` | Green on `main` (34 tests) |
| GBA ROM build | Release workflow `build-rom` | Green |
| Graphics lint | `validate_graphics.py` in host tests | Green |
| Audio lint | `gen_audio.py --check` | Green |
| Release ROM committed | `rom/thomas_bridge.gba` | Present |

---

## Screen-flow scenarios (M1–M5)

| ID | Flow | Expected | Evidence |
|----|------|----------|----------|
| **M1** | Title → Level 1 → Build → Test → Win | HATT OK / PEEP | **Simulation:** `test_physics` Knapford reference playthrough succeeds with 3★. **Web E2E:** `ship-qa.spec.ts` builds reference bridge and completes test run (`M1: Knapford reference bridge wins`). |
| **M2** | Level 2 → weak bridge → Test | DELAY / ASHES | **Simulation:** `test_physics` overlong span snaps; Gordon 10-weight fails two-span deck. **Web E2E:** `ship-qa.spec.ts` START→END span fails on Knapford (`M2: overlong Knapford span fails test run`). |
| **M3** | Build with no connections → Start | NO GO → back to build | **Web E2E:** `ship-qa.spec.ts` M3 |
| **M4** | Pause → Restart / Levels / Title | Correct transitions | **Web E2E:** `ship-qa.spec.ts` M4 (pause/resume, levels, title) |
| **M5** | Web theater + resize | Canvas scales, no clip | **Web E2E:** `ship-qa.spec.ts` M5 |

---

## Manual checklist (emulator / hardware)

Use [testing.md](testing.md) for the full GBA and web checklist. Recommended order before EverDrive deploy:

1. `make -C tests`
2. `./scripts/sync_rom_to_web.sh && cd web && npm run test:e2e`
3. mGBA desktop or web playtest — confirm M1 win on Knapford with reference bridge
4. EverDrive GBA — copy `rom/thomas_bridge.gba` to SD `GBA/` folder ([hardware.md](hardware.md))

### GBA / emulator spot checks

- [x] Title hints (A NOD, START) — implemented (`LABEL_HINT_*`)
- [x] Level select track names + subtitles (FLAT!/HILL!/LONG!)
- [x] Build hints rotate → PAUSE
- [x] Track vs brace tools (Gordon's Hill+)
- [x] NO GO auto-return on test
- [x] Pause menu (Select)
- [x] Mode music via AudioDirector (Island Line / Yard / Workshop / Full Steam)
- [x] Chuff sync on test run + idle on engine select
- [x] Per-level BG themes (knapford / hill / viaduct tilesets)
- [x] Per-engine idle motif on engine select (yard.mod patterns 4–13)
- [x] Win/fail stings (Useful+Whistle / snap+delay+splash)
- [x] SRAM stars + intro_seen (host `test_save_data`, `test_progress`)

### Web playtest

- [x] ROM loads without error banner (E2E `rom-boot.spec.ts`)
- [x] COOP/COEP isolation for WASM threads
- [x] Command queue + theater mode (E2E)
- [x] Canvas 240×160 render

### Docker

- [x] Release workflow builds and pushes image after ROM + tests
- [ ] Local `docker build` — run before CapRover deploy if used

---

## Walkthrough artifact

Captured via Playwright (`web/e2e/walkthrough-capture.spec.ts`):

```bash
./scripts/sync_rom_to_web.sh
cd web && WALKTHROUGH_DIR=../artifacts/walkthrough npm run test:e2e -- e2e/walkthrough-capture.spec.ts
```

| Screenshot | Screen |
|------------|--------|
| `01_title_screen` | Island Line title |
| `02_level_select` | PICK TRACK — **KNAPFORD / FLAT!** track card (64×32) |
| `03_engine_select` | PICK ENGINE + Thomas portrait |
| `04_knapford_build` | BUILD mode + meadow theme |
| `05_gordons_hill_select` | PICK TRACK on Gordon's Hill (save unlock) |
| `06_gordons_hill_build_track` | BUILD on hill theme (track tool) |
| `07_gordons_hill_build_rod` | ROD/brace tool toggled (R button) |
| `08_vicarstown_select` | PICK TRACK on Vicarstown (all_unlocked save) |
| `09_vicarstown_build` | BUILD on viaduct theme (James) |

- Web playtest seeds Gordon's Hill unlock via `?walkthrough=gordon` (`uploadSaveOrSaveState` before first `loadGame`; E2E `gordon-save.spec.ts`). Vicarstown uses `?walkthrough=vicarstown` with `all_unlocked.sav`.

Gordon's Hill / Vicarstown themed BG and brace tools require SRAM unlock (clear prior track). Host `run_level_playthrough(1|2)` proves 3★ reference wins on those levels.

**Reference win build (M1 simulation source of truth):** `include/level_solutions.h` → `build_knapford_reference()` — must match host `run_level_playthrough(0)`.

---

## EverDrive hardware sign-off (pending physical device)

Run on real GBA / EverDrive after copying `rom/thomas_bridge.gba` (340,892 bytes) to SD `GBA/`:

| Step | Check | Pass? | Web proxy |
|------|-------|-------|-----------|
| H1 | ROM boots to title; no crash in 60 s attract loop | | `hardware-qa-proxy.spec.ts` (15 s attract) |
| H2 | Level select → Knapford build → test → reference win → HATT OK | | `hardware-qa-proxy.spec.ts` H2 |
| H3 | Clear Knapford → Gordon's Hill unlocks; hill BG theme visible; brace tool works | | `hardware-qa-proxy.spec.ts` H3 + `gordon-save.spec.ts` |
| H4 | Engine select plays per-engine yard motif when scrolling | | `hardware-qa-proxy.spec.ts` H4 (visual only; audio on hardware) |
| H5 | Pause (Select) ducks music; resume restores volume | | `hardware-qa-proxy.spec.ts` H5 (menu flow; duck on hardware) |
| H6 | Power off/on — SRAM stars + intro_seen persist | | `sram-persist.spec.ts` (soft reset only) |

Web E2E proxy for H6: `web/e2e/sram-persist.spec.ts` verifies `intro_seen_mask`, stars, and clear flags survive mGBA soft reset (walkthrough saves).

Record results in this file or a hardware test log when complete.

### Delta emulator (iOS)

1. **Import ROM:** AirDrop or Files → import `rom/thomas_bridge.gba` into Delta. Or download **`thomas_bridge.gba`** from the [v1.0.0 draft release](https://github.com/hackmods/morris-bridge-gba/releases) on GitHub (publish after H1–H6 pass).
2. **Fresh SRAM (H1/H2):** Long-press the game → delete save data, or remove any existing `thomas_bridge.sav` before first boot.
3. **H3 shortcut (optional):** Skip manual Knapford clear by seeding a walkthrough save:
   ```bash
   python3 tools/gen_playtest_save.py
   ```
   Import `web/public/saves/gordon_hill_walkthrough.sav` into Delta and rename it to **`thomas_bridge.sav`** (same basename as the ROM). Boot → Gordon's Hill should appear on level select with Knapford already cleared.

   Or run `./scripts/export_delta_saves.sh` — copies pre-named saves to `rom/delta_saves/` for import.
4. **H6 power-cycle:** After a win, **force-quit Delta** (app switcher) — do not use Delta's in-app reset alone. Relaunch and confirm stars / unlocks / intro flags persisted.

mGBA desktop uses the same **`<rom_basename>.sav`** naming (`thomas_bridge.sav`).

### Reference builds for H2 (Delta / EverDrive)

Source of truth: `include/level_solutions.h`. Minimum win path for **H2 (Knapford)**:

1. Title → **START** → Level select → **A** on Knapford → Engine select → **A** on Thomas
2. Build mode: place **two mid-deck nodes** on the flat track line, then connect **START → mid-left → mid-right → END** with the track tool (**A** on each pair). Three spans total.
3. **START** to test → wait for train to reach END → expect **HATT OK** / **PEEP!**

For **H3**, complete H2 first (SRAM records clear). On next visit, Gordon's Hill unlocks on level select. Use **R** to toggle **ROD** and add bank anchors to START/END per `build_gordon_hill_reference()` in `level_solutions.h`. Hill BG theme should appear in build mode.

Web E2E builds the same layouts via `web/e2e/helpers.ts` (`buildKnapfordReferenceBridge`, etc.) — use the web playtest command queue if you want to rehearse before hardware.

---

## Sign-off criteria (Phase 7)

| Requirement | Met? |
|-------------|------|
| Release ROM in repo | Yes |
| Changelog updated | Yes |
| CI pyramid green | Yes |
| M3–M5 E2E | Yes |
| M1/M2 simulation playthroughs | Yes |
| M1 web win E2E | Yes (`ship-qa.spec.ts`) |
| M2 web fail E2E | Yes (`ship-qa.spec.ts`) |
| Gordon Hill web win E2E | Yes (`ship-qa.spec.ts` + `?walkthrough=gordon`) |
| Vicarstown web win E2E | Yes (`ship-qa.spec.ts` + `?walkthrough=vicarstown`) |
| Hardware QA web proxies | Yes (`hardware-qa-proxy.spec.ts` H1–H5; `sram-persist.spec.ts` H6 soft reset) |
| Hardware EverDrive smoke | Pending physical device — see H1–H6 checklist below |
| Walkthrough artifact | `artifacts/walkthrough/` (9 PNGs) + `web/e2e/walkthrough-capture.spec.ts` |

Post-ship optional: ghost replay, challenge flags (master-plan §13).
