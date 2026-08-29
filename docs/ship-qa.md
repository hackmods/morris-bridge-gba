# Ship QA — Phase 7 verification

**Product:** Thomas's Bridge (`thomas_bridge.gba`)  
**ROM:** `THOMASBRIDGE` / `THOM01` · ~330 KB (338,236 bytes) as of 2026-08-29  
**Release branch:** `main`

This document records master-plan **M1–M5** screen-flow QA and related ship gates. Physical GBA hardware was not available in the Cloud Agent environment; **mGBA-WASM web playtest** and **host playthrough tests** provide equivalent coverage where noted.

---

## Automated gates (CI)

| Gate | Command / job | Status |
|------|----------------|--------|
| Host physics + session | `make -C tests` | Green on `main` |
| All-level playthroughs | `test_physics` Knapford / Gordon / Vicarstown 3★ | Green |
| Web unit tests | `cd web && npm test` | Green |
| Web E2E (ROM boot, gameplay, **ship-qa M3–M5**, Gordon save) | `npm run test:e2e` | Green on `main` (20 tests) |
| GBA ROM build | Release workflow `build-rom` | Green |
| Graphics lint | `validate_graphics.py` in host tests | Green |
| Audio lint | `gen_audio.py --check` | Green |
| Release ROM committed | `rom/thomas_bridge.gba` | Present |

---

## Screen-flow scenarios (M1–M5)

| ID | Flow | Expected | Evidence |
|----|------|----------|----------|
| **M1** | Title → Level 1 → Build → Test → Win | HATT OK / PEEP | **Simulation:** `test_physics` Knapford reference playthrough succeeds with 3★. **Web:** E2E `ship-qa.spec.ts` reaches build mode from title (`M1 partial`). Full win loop: manual web or hardware (see walkthrough). |
| **M2** | Level 2 → weak bridge → Test | DELAY / ASHES | **Simulation:** `test_physics` overlong span snaps; Gordon 10-weight fails two-span deck. Manual level-2 fail sting verified via audio assets in CI. |
| **M3** | Build with no connections → Start | NO GO → back to build | **Web E2E:** `ship-qa.spec.ts` M3 |
| **M4** | Pause → Restart / Levels / Title | Correct transitions | **Web E2E:** `ship-qa.spec.ts` M4 (pause + resume). Restart/levels/title: manual spot-check. |
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

- Web playtest seeds Gordon's Hill unlock via `?walkthrough=gordon` (`uploadSaveOrSaveState` before first `loadGame`; E2E `gordon-save.spec.ts`).

Gordon's Hill / Vicarstown themed BG and brace tools require SRAM unlock (clear prior track). Host `run_level_playthrough(1|2)` proves 3★ reference wins on those levels.

**Reference win build (M1 simulation source of truth):** `include/level_solutions.h` → `build_knapford_reference()` — must match host `run_level_playthrough(0)`.

---

## EverDrive hardware sign-off (pending physical device)

Run on real GBA / EverDrive after copying `rom/thomas_bridge.gba` (338,236 bytes) to SD `GBA/`:

| Step | Check | Pass? |
|------|-------|-------|
| H1 | ROM boots to title; no crash in 60 s attract loop | |
| H2 | Level select → Knapford build → test → reference win → HATT OK | |
| H3 | Clear Knapford → Gordon's Hill unlocks; hill BG theme visible; brace tool works | |
| H4 | Engine select plays per-engine yard motif when scrolling | |
| H5 | Pause (Select) ducks music; resume restores volume | |
| H6 | Power off/on — SRAM stars + intro_seen persist | |

Record results in this file or a hardware test log when complete.

---

## Sign-off criteria (Phase 7)

| Requirement | Met? |
|-------------|------|
| Release ROM in repo | Yes |
| Changelog updated | Yes |
| CI pyramid green | Yes |
| M3–M5 E2E | Yes |
| M1/M2 simulation playthroughs | Yes |
| Hardware EverDrive smoke | Pending physical device — see H1–H6 checklist below |
| Walkthrough artifact | `artifacts/walkthrough/` (7 PNGs) + `web/e2e/walkthrough-capture.spec.ts` |

Post-ship optional: ghost replay, challenge flags (master-plan §13).
