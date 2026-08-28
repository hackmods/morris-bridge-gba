# Physics (fixed-point)

All game physics uses Butano **`bn::fixed`** (20.12: 20 integer bits, 12 fractional bits, `1.0 = 4096`). No `float` or `double` on the GBA path.

Host unit tests compile the same [`physics.cpp`](../src/physics.cpp) against a 20.12 mock in [`tests/host_bn/`](../tests/host_bn/).

## Distance

**Squared distance** (hot path, cursor snap):

```text
dx = a.x - b.x
dy = a.y - b.y
squared_distance = dx² + dy²
```

Compare against `radius²` to avoid `sqrt` when snapping to nodes.

**Euclidean distance** uses a restoring **integer square root** on 64-bit intermediates (`isqrt64`), then scales back to 20.12 (`approx_sqrt`). No BIOS `Sqrt` SWI, no libm.

## Tension

When the train is on a strut:

```text
Tension = BaseWeight × (DistanceFromTrain / StrutLength)
```

- `DistanceFromTrain` = distance along the strut from the start node toward the train
- At the far end of the strut, tension equals `BaseWeight`
- At the near end, tension approaches 0
- Degenerate strut (`length == 0`) → tension 0

Implementation: [`physics::tension()`](../include/physics.h)

## Max tension (span rating)

New struts get a length-dependent limit:

```text
max_tension = BASE_MAX_TENSION × SAFE_LENGTH / length
```

Clamped to minimum 1. Examples (defaults `BASE_MAX_TENSION=8`, `SAFE_LENGTH=48`):

| Span length | max_tension |
|-------------|-------------|
| 48 px | 8.0 |
| 96 px | 4.0 |
| 176 px | ~2.2 (snaps under `BASE_WEIGHT=4`) |

## Failure

Each frame on the active strut:

```text
if current_tension > strut.max_tension:
    break strut
    train enters fall state (vy += GRAVITY each frame)
```

Fall continues until `train.y > FALL_Y_LIMIT` (off bottom of 160px screen) → `failed_fall`.

## Interpolation

Train position along a strut uses [`physics::lerp()`](../include/physics.h) with parameter `t ∈ [0, 1]`.

## Host test coverage

[`tests/test_physics.cpp`](../tests/test_physics.cpp) asserts:

- 3-4-5 triangle distance via `approx_sqrt`
- Tension at start, midspan, and far end of a strut
- Two-span bridge carries train (`success`)
- Single overlong span snaps (`failed_fall`)
- Disconnected banks → `no_path`

Run: `make -C tests`
