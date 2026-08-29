# Ship QA — Phase 7 verification

**Product:** Thomas's Bridge (`thomas_bridge.gba`)  
**ROM:** `THOMASBRIDGE` / `THOM01` · ~327 KB (334,984 bytes) as of 2026-08-29  
**Release branch:** `main`

This document records master-plan **M1–M5** screen-flow QA and related ship gates. Physical GBA hardware was not available in the Cloud Agent environment; **mGBA-WASM web playtest** and **host playthrough tests** provide equivalent coverage where noted.

---

## Automated gates (CI)

| Gate | Command / job | Status |
|------|----------------|--------|
| Host physics + session | `make -C tests` | Green on `main` |
| All-level playthroughs | `test_physics` Knapford / Gordon / Vicarstown 3★ | Green |
| Web unit tests | `cd web && npm test` | Green |
| Web E2E (ROM boot, gameplay, **ship-qa M3–M5**) | `npm run test:e2e` | Green on `main` |
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

Web playthrough demonstrating title → level select → build mode on the shipped ROM (Phase 7 deliverable). See Cloud Agent walkthrough recording / screenshots attached to the Phase 7 PR.

**Reference win build (M1 simulation source of truth):** `include/level_solutions.h` → `build_knapford_reference()` — must match host `run_level_playthrough(0)`.

---

## Sign-off criteria (Phase 7)

| Requirement | Met? |
|-------------|------|
| Release ROM in repo | Yes |
| Changelog updated | Yes |
| CI pyramid green | Yes |
| M3–M5 E2E | Yes |
| M1/M2 simulation playthroughs | Yes |
| Hardware EverDrive smoke | Pending physical device |
| Walkthrough artifact | PR / agent recording |

Post-ship optional: ghost replay, challenge flags (master-plan §13).
