# Physics (fixed-point)

Test mode runs a real structural simulation: a **pin-jointed spring truss**
solved in integers on the GBA. A link holds or snaps because of the force that
actually reaches it — nothing is rated up front and nothing gets a bonus for
how many neighbours it has.

No `float`, no `double`, no heap. Positions are Butano **`bn::fixed`** (20.12);
the solver's inner loop is raw `int32_t`/`int64_t`. Host unit tests compile the
same sources against a 20.12 mock in [`tests/host_bn/`](../tests/host_bn/), so
host and ROM produce the same numbers.

Code: [`include/bridge_system.h`](../include/bridge_system.h),
[`src/core/bridge_system.cpp`](../src/core/bridge_system.cpp).

## Model

| Thing | Behaviour |
|-------|-----------|
| **Anchor** | Bedrock. Never moves, takes any force. |
| **Joint** | Free point with weight: `JOINT_WEIGHT` (¼) plus half of every link welded to it. |
| **Link** | Axial spring. `force = stiffness × strain`. Snaps when `|force| > strength`, in tension or compression. |
| **Train** | Axle loads (engine + cars 26 px apart) shared between the two joints of the link each axle stands on, by lever rule. |

Forces are in engine-weight units — Thomas weighs 4.

| Material | Role | Stiffness (EA) | Strength | Weight / px | Cost | Reach |
|----------|------|----------------|----------|-------------|------|-------|
| Timber | Track (routable) | 480 | 8 | 1/128 | 1 | level beam length |
| Rod | Brace | 720 | 12 | 1/128 | 2 | 1.5 × beam length |

Consequences that fall out of the model rather than being scripted:

- A joint welded to nothing falls.
- A deck strung straight from bank to bank is a chain, not a beam: it sags and
  the tension that sag creates snaps it. The tension in a sagging chain is about
  `cbrt(W² × EA / 8)`, which exceeds timber strength for every engine.
- A joint tied to two anchors by two non-parallel links cannot move.
- Shallow props and stays carry far more than the load they hold up
  (`load / sin(angle)`); steep ones carry about the load itself.
- A link between two anchors carries nothing. It still costs.
- More structure is more dead weight for the rest to carry.

## Solver

Each frame runs `SIM_SUBSTEPS` (4) explicit substeps:

1. Every joint gets its weight plus any axle load.
2. Every link adds its axial force to both ends. Strain comes from squared
   lengths, `x = d²/rest² − 1`, `strain ≈ x/2 − x²/8 + x³/16`, using reciprocals
   precomputed in `begin_test()` — **no square root and no division per link**.
3. Joints integrate (velocity, damping, position). Gravity is 1/64 px per
   substep², i.e. the same ¼ px per frame² the falling train uses.

**Stability.** Explicit integration needs `stiffness / inertia` bounded at every
joint. Joints keep their true weight for gravity but borrow extra *inertia*
where stiff links meet (`_refresh_inertia`). Static equilibrium — which is what
decides a break — does not involve inertia, so the answer is unchanged; only
how fast the structure rings is.

**Settling.** For `SETTLE_FRAMES` (40) the train waits on the bank while
self-weight fades in over `GRAVITY_RAMP_FRAMES` (24) under heavier damping. A
bridge that cannot hold itself up comes down before the engine moves.

## Routing

`begin_test()` picks the **shortest** route by length over track links
(Dijkstra, ≤16 nodes). Track steeper than 1-in-`MAX_GRADE_RUN` (1 in 2) is not
a route — engines stall on it — so timber can be used as a post or strut
without the train trying to drive down it, and a timber truss chord above the
deck never steals the train from the deck.

## Build limits

- Timber no longer than the level's beam length (`level::Def::safe_length`,
  44–48 px); rod up to 1.5× that. The weld ghost turns red past the limit.
- No link shorter than `MIN_LINK_LENGTH` (8 px).
- 16 nodes including anchors, 32 links.

## Failure and success

- **Snap:** the link is removed, its stored energy is released into the
  neighbours, and the collapse plays out physically.
- **Fall:** the link under the engine or any coach is gone, or one of them has
  sagged more than `SINK_DEPTH` (20 px) below the line as built.
- **Success:** the last coach is off the bridge. The engine runs on down the
  far bank until it is.

## The rewind

`end_test()` restores the build exactly as welded. Each link keeps
`peak_tension`, the worst force it carried; a link that snapped reads as full.
Build mode heats links at ≥ 75 % (`strut_was_overworked`), so after a failed
run the bridge shows where it gave way.

## Engines and trains

| Engine | Weight | Cars (weight) | Total | Speed (px/frame) |
|--------|--------|---------------|-------|------------------|
| Percy | 3 | truck (1) | 4 | 1 |
| Thomas | 4 | Annie (1), Clarabel (1) | 6 | 1 |
| Edward | 4 | tender (2), coach (1) | 7 | ¾ |
| James | 5 | tender (2), coach (1) | 8 | 1¼ |
| Henry | 5 | tender (2), truck (1), truck (1) | 9 | 1 |
| Gordon | 8 | tender (2), coach (2), coach (2) | 14 | 1½ |

Each vehicle is one axle load, `CAR_SPACING` (26 px) behind the one in front,
drawn at `BridgeSystem::axle_position`. The train starts parked on the start bank.

- **Trucks** (`CarKind::truck`) jolt: their load is `weight × (1 ± TRUCK_JOLT)`,
  flipping every `TRUCK_JOLT_FRAMES`, alternating truck to truck.
- **Banker:** on a level with `banked` set, Edward is added as a last car of
  `BANKER_WEIGHT` (4) behind every train.
- **Grade:** speed is multiplied by `1 − rise/run` of the link under the engine
  (clamped ½ to 1¼), measured on the bridge as it currently hangs.
- Speed ramps from `ENGINE_MIN_SPEED` over `ACCEL_FRAMES` (24).

## Layout

| Constant | Value | Meaning |
|----------|-------|---------|
| `level::Def::deck_y` | per level | Rail height of both banks |
| `GROUND_TILE_Y − TRACK_Y` | 4 | Bank ground sits this far below the rail |
| `TRAIN_Y_OFFSET` | −6 | Sprite offset so wheels sit on the rail |

## Host test coverage

Run: `make -C tests`. See [Testing](testing.md).

The structural tests are bridge-builder invariants, checked on all eight levels:

- an unsupported deck collapses;
- the par design holds, with the scheduled engine, at par cost;
- removing **any one** link from the par design loses the train;
- Gordon breaks every par design, except on Gordon's Hill where he is banked;
- weld order does not change the result;
- a link between two anchors carries zero force.
