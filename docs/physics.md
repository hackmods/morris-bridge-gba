# Physics (fixed-point)

All game physics uses Butano **`bn::fixed`** (20.12). No `float` or `double` on the GBA path.

Host unit tests compile the same sources against a 20.12 mock in [`tests/host_bn/`](../tests/host_bn/).

## Layout

| Constant | Value | Meaning |
|----------|-------|---------|
| `TRACK_Y` | 52 | Rail height — nodes, start/end banks |
| `GROUND_TILE_Y` | 56 | Ground scenery tile centers |
| `TRAIN_Y_OFFSET` | −6 | Sprite offset so wheels sit on the rail |

## Distance

Squared distance avoids `sqrt` in cursor snap. Euclidean distance uses `isqrt64` + `approx_sqrt`.

## Tension

```text
Tension = Load × (DistanceFromTrain / StrutLength)
```

- **Cars:** `BASE_WEIGHT` (4)
- **Locomotive** on the first span: `ENGINE_WEIGHT` (6)

## Max tension (span rating)

Base rating (inverse length):

```text
base = BASE_MAX_TENSION × SAFE_LENGTH / length
```

Effective rating after structural bonuses:

```text
effective = base × reinforcement × (triangulated ? TRIANGLE_BONUS : 1)
```

**Reinforcement:** each redundant strut at an endpoint adds `REINFORCEMENT_PER_JOINT` (¼) to the multiplier.

**Triangle bonus:** when a third node forms a closed triangle with the strut, multiply by `TRIANGLE_BONUS` (1.5×).

## Locomotive engine

- Speed ramps from `ENGINE_MIN_SPEED` to `TRAIN_SPEED` over `ACCEL_FRAMES` (24 frames).
- Heavier load on span index 0 (`ENGINE_WEIGHT`).

## Failure

When `current_tension > max_tension`, the strut breaks, the train keeps its lerp position, and fall begins with `FALL_INITIAL_VY` plus `GRAVITY` each frame.

## Audio

Original parody loops via Maxmod (`AUDIOBACKEND := maxmod`):

| Track | Cue |
|-------|-----|
| `island_line.mod` | Title march |
| `yard.mod` | Level select |
| `workshop.mod` | Build mode |
| `full_steam.mod` | Crossing |

SFX: `chuff`, `peep`, `whistle`, `useful`, `snap`, `delay`, `splash`, `ui`.

See [Audio](audio.md). Generate with `python3 tools/gen_audio.py`.

## Host test coverage

Run: `make -C tests`
