# Diagnosis — demo vs vision (Sodor Bridge Builder)

Review date: 2026-09-24. Product remains the GBA ROM (`rom/thomas_bridge.gba`); web stays mGBA-WASM playtest ([AGENTS.md](../AGENTS.md)).

## What was broken / incomplete

| Area | Before | After this refactor |
|------|--------|---------------------|
| Materials | Implicit timber/rod tied to track/brace role; integer cost 1/2 | Wood £10 / Iron £25 / Stone £50 / Cable £15 (`include/material.h`) |
| Budget | Cost vs par digits only | Level £ purse (`budget_pounds`) with live remaining/spent HUD |
| Grid | 4×4 snap | **8×8** snap (`GRID_STEP`) |
| Physics | Path-strut tension only; braces only boosted max rating | Load-path truss solve: deck force + stiffness-weighted redistribute to incident members; `stress = F / yield`; cascade snap |
| Collapse | Single snap → fall | Cascade re-solve (up to 4 passes); train → `TrainState::derailed` |
| Stress look | Binary idle/hot red | 4-band heatmap: green / yellow / red / magenta flash |
| Controls | R=role, L=undo, B=delete node | **A draw, B delete beam (or node), L/R cycle material, START run** |
| States | `build` / `test` + result screens | Same flow; aliases `edit` / `simulation`; `TrainState` + documented EDIT/SIM/RESULT |
| Failure copy | Random ASHES / lore | Prefer **CONFUSION** / **DELAY** + furious Hatt |

## Architecture (modular)

```
content/     level defs + engines
material.h   £ costs, yield, tension-only / pier flags
physics.*    distance, tension, rated_yield, stress_ratio, stress_band
core/bridge_system.*   graph, EDIT edits, SIM truss solve + cascade, RESULT status
session/     RunScore (£)
platform     Game + screens + GraphView (heatmap) + BGView
```

## Remaining vision polish (not blocking usable loop)

- Right-side vertical material icon strip (OAM); L/R cycles WOOD/IRON/STONE/CABLE labels today.
- Clearer mid-SIM stress-heatmap capture when 8×8 strut helpers land consistently in playwright.
- Full dense FEM matrix — intentionally replaced by documented load-path truss (GBA frame budget).

**Top chrome now shows** EDIT + BUDGT + 4-digit £ + target load. **Bottom strip** shows material + A DRAW/START.

See [physics.md](physics.md) for tuneable yield / cascade knobs.
