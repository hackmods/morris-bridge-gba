# Physics (fixed-point truss stress)

All game physics uses Butano **`bn::fixed`** (20.12). No `float`/`double` on the GBA path. Host tests compile the same sources against `tests/host_bn/`.

## Why not full FEM?

A dense direct-stiffness solve (up to 16 nodes × 2 DOF) exceeds the ≤15% simulation frame budget once you include sprite sync. We use a **load-path truss approximation** that still exposes per-material yield and cascade collapse.

## Materials (`include/material.h`)

| Id | £ | Yield (base) | Notes |
|----|---|--------------|-------|
| Wood | 10 | 8 | Deck default |
| Iron | 25 | 14 | Girder / brace |
| Stone | 50 | ∞ (1000) | Pier only — one end must be a ground anchor |
| Cable | 15 | 7 | Tension-only (compression share discarded) |

Joints cost £10 each (`NODE_COST`).

## Rated yield

```text
rated = material_yield × (safe_length / length)
      × reinforcement(joints_a, joints_b)
      × (triangulated ? 1.5 : 1)
```

Stone skips length derating. Collapse when `stress >= 1.0`.

## Load-path solve (each SIM frame on the active track member)

1. Train load `W` (engine + coach ramp) → axial deck force via classic
   `tension = W × (distance_along / length)`.
2. Split `W/2` to each endpoint; redistribute to **other** incident members
   proportional to `stiffness_mul / length` (relative EA/L).
3. `stress_i = |F_i| / rated_yield_i`.
4. If any `stress >= 1`, destroy the worst member, re-solve (≤ `CASCADE_MAX_PASSES`),
   and if the train’s path member is gone → **DERAILED** (`failed_fall`).

## Stress heatmap bands

| Band | Stress | Palette |
|------|--------|---------|
| Green | 0–50% | cool green |
| Yellow | 51–80% | amber |
| Red | 81–99% | hot red |
| Breaking | ≥100% | magenta / snap frame |

## Tuning difficulty

- Raise `material::TABLE[].yield_strength` → easier.
- Lower `SAFE_LENGTH` / level `safe_length` → harsher long spans.
- Increase train `ENGINE_WEIGHT` / coaches → harder.
- `CASCADE_MAX_PASSES` limits how far a collapse propagates per frame.

## Legacy helpers

`tension`, `effective_max_tension`, and `exceeds_max_tension` remain for ratings and older tests. Simulation prefers `stress_ratio` / `is_breaking`.

## Layout constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `TRACK_Y` | 52 | Rail height |
| `GRID_STEP` | 8 | EDIT_MODE snap |
| `UNDERGROUND_Y` | 64 | Pier build floor |
