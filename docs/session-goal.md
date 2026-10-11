# Session Goal — Ship Thomas's Bridge (Engine v2 → Gold)

Copy everything inside the **GOAL** block below into a new Cloud Agent session (or local Cursor chat) to drive full implementation. Read [master-plan.md](master-plan.md) first.

**For the immediate UI / playability / mockup-look track**, prefer the shorter goal in [ui-playability-plan.md](ui-playability-plan.md) §9 (menus → feel → chrome → Knapford art). Use this full Gold goal when that track’s exit criteria are met.

---

## GOAL

**Finish Thomas's Bridge as a shippable Game Boy Advance product** — not a prototype, and not a browser workshop. The whole point of this repository is the GBA ROM. Implement [master-plan.md](master-plan.md) Phases 1–7 in order. Merge or build on PR #13 (backdrop/level/playthrough fixes) if not yet on `main`.

Do **not** ship a high-resolution web builder as the product, or demote the ROM to “legacy.” Web stays the mGBA playtest. See [AGENTS.md](../AGENTS.md).

### Definition of done

1. **Simulation:** `LinkRole` (track / brace / foundation-ready), track-only pathfinding, `BuildSession` with tools (place, track, brace, erase, undo), `MaterialId` (timber + rod), `RunScore` with par and ★.
2. **Architecture:** `core/`, `session/`, `platform/gba/screens/`, `GraphView`, `AudioDirector`; `main.cpp` orchestration only (~400 lines max).
3. **Content:** 8 data-driven levels (Knapford through Vicarstown) with tool unlock progression; reference solution + `run_level_playthrough` host test for **each** level.
4. **Graphics:** Move sky/water/hills to **BG parallax** per theme; replace placeholder sprites with **authored** 32×32 trains, deck beams, rod braces, nodes, Hatt; stress palette feedback; juice (shake, dust, snap crack).
5. **Audio:** Polished mode loops (Island Line, Yard, Workshop, Full Steam); frame-locked chuff; win/fail stings; pause ducking via AudioDirector.
6. **Lore:** Optional first-visit intro lines per level + rare HUD strings from [lore-leak.md](lore-leak.md); epilogue beat on all-clear — never blocking gameplay.
7. **Meta:** SRAM v4 with best ★ and cost per level; results screen shows stars; level select displays best score.
8. **Testing:** `make -C tests` green (physics + session + all playthroughs); `npm test` green; `validate_graphics.py` + `gen_audio.py --check`; manual M1–M5 on mGBA; EverDrive smoke if available.
9. **Ship:** Release ROM synced to web; changelog updated; demo walkthrough (screens or recording) proving build → test → win loop on Gordon's Hill with braces.

### Execution rules

- **Vertical slice first:** Do not add level 4 or new engines until Gordon's Hill works end-to-end with track + brace, new art on that scene, and a passing host test.
- **No float, no heap** in simulation; host tests remain the physics source of truth.
- **Do not tie materials to engine type.**
- **Do not overwrite authored BMPs** with `gen_graphics.py` — switch generator to validate-only once art lands.
- Commit and push each phase; open/update PR; run tests before every push.

### Suggested first prompt for the new session

> Read `docs/master-plan.md` and `docs/session-goal.md`. Start Phase 1: implement `LinkRole`, track-only `RouteFinder`, `BuildTool` track vs brace, two link sprites, and Gordon's Hill as the golden vertical slice. Extract simulation into `src/core/` and session into `src/session/` without breaking host tests. When Phase 1 passes `make -C tests` and manual mGBA check on hill level, continue through Phases 2–7 until the session goal definition of done is met.

---

## Quick reference

| Doc | Purpose |
|-----|---------|
| [master-plan.md](master-plan.md) | Full architecture, phases, AV spec |
| [lore-leak.md](lore-leak.md) | Optional PG flavor text |
| [testing.md](testing.md) | Manual QA checklist |
| [graphics.md](graphics.md) | Palette and sprite rules |
| [audio.md](audio.md) | Cue map and formats |
| [hardware.md](hardware.md) | EverDrive / GBA verification |
