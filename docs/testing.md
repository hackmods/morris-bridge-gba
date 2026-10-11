# Testing

## Host physics tests (recommended before every ROM deploy)

Compiles real [`physics.cpp`](../src/core/physics.cpp) and [`bridge_system.cpp`](../src/core/bridge_system.cpp) against a **20.12 `bn::fixed` mock** — no Butano, no emulator, no floating-point.

```bash
make -C tests
```

### What is tested

| Category | Examples |
|----------|----------|
| Math | `isqrt64`, `approx_sqrt`, 3-4-5 distance |
| Build rules | Beam/rod reach, minimum length, duplicates, locked tools, undo, joint budget |
| Routing | `no_path`, rods are not a route, grade limit, deck preferred over a truss chord |
| Structure (every level) | Unsupported deck collapses; par design holds; removing any one link loses the train; Gordon breaks par; weld order is irrelevant |
| Structure (spot checks) | Loose joints fall, anchor-to-anchor links carry nothing, the Knapford post carries the engine's weight |
| Rewind | A failed test restores the build and marks the links that gave way |
| Scoring | Par cost = 3★ on the scheduled engine, lighter engine = 1★, over-building loses stars |
| Levels | Per-level seed counts, START/END positions, invalid index clamping |

### QA ROM with the par designs pre-built

Driving the build cursor is slow when all you want is to look at a bridge on
the GBA. Building with `MORRIS_PRELOAD_REFERENCE` starts every track with its
par design already welded:

```bash
make LIBBUTANO=vendor/butano/butano   USERFLAGS="-DBN_CFG_SPRITE_AFFINE_MATS_MAX_ITEMS=32 -DBN_CFG_SPRITES_MAX_SORT_LAYERS=32 -DMORRIS_PRELOAD_REFERENCE"
```

Never ship that ROM. Touch `src/game.cpp` and rebuild without the flag before
copying to `rom/`.

### Mock layer

[`tests/host_bn/`](../tests/host_bn/) provides minimal stand-ins:

- `bn_fixed.h` — 20.12 arithmetic
- `bn_fixed_point.h`
- `bn_vector.h`
- `bn_optional.h`

## Web playtest tests

Vitest covers the command queue parser and responsive canvas scaling helpers.

```bash
cd web
npm ci
npm test
```

| File | Scenarios |
|------|-----------|
| `web/src/input-queue.test.ts` | Token parsing, unknown commands, queue pause/clear |
| `web/src/layout.test.ts` | Canvas scale math, max scale cap, GBA dimensions |

## Web playtest E2E (Playwright)

End-to-end tests boot the real mGBA-WASM emulator in Chromium, load `thomas_bridge.gba`, and verify rendering plus basic gameplay flows.

**Prerequisite:** sync a built ROM into the web public folder:

```bash
./scripts/sync_rom_to_web.sh   # copies rom/thomas_bridge.gba → web/public/rom/
```

Run E2E locally:

```bash
cd web
npm ci
npx playwright install chromium
npm run test:e2e
```

| Spec | Scenarios |
|------|-----------|
| `web/e2e/rom-boot.spec.ts` | ROM load, COOP/COEP isolation, canvas screenshot after boot |
| `web/e2e/rendering.spec.ts` | Canvas screenshot checks, soft reset, PNG payload size |
| `web/e2e/command-queue.spec.ts` | Enqueue Start, multi-token input, pause/clear |
| `web/e2e/gameplay.spec.ts` | Title → level → build via Start/queue, theater mode |

Dev builds expose `window.__TEST__` hooks (`web/src/test-hooks.ts`) for frame waits and canvas sampling. Production builds omit these hooks.

CI runs E2E in the **Release** workflow (`web-e2e` job) after the ROM artifact is built.

## CI integration

The **Release** workflow runs:

1. `make -C tests` in the `host-tests` job
2. `npm test` in the `web-tests` job
3. `npm run test:e2e` in the `web-e2e` job (Playwright + ROM artifact)
4. Docker image build after all three pass

See [CI/CD](ci-cd.md).

**Phase 7 ship QA:** [ship-qa.md](ship-qa.md) — M1–M5 flows, CI gates, hardware sign-off.

## Manual QA checklist

### GBA / emulator

- [ ] Title shows **HOLD A**, **A WELD**, and blinking **START**
- [ ] Level select shows track names without overlapping labels
- [ ] First build shows rotating hints, then **PAUSE**
- [ ] D-Pad moves cursor; R speeds it up
- [ ] A places nodes and welds (hold A and drag, or tap between joints)
- [ ] Start runs train; short bridge succeeds
- [ ] Long single span snaps; train falls
- [ ] No struts → "NO GO" / no path
- [ ] Select opens pause menu
- [ ] Title plays Island Line; level select plays Yard
- [ ] Build mode plays Workshop; Start plays peep + Full Steam
- [ ] Crossing chuffs in time with the sprite
- [ ] Success plays Useful + whistle; fail plays snap / delay / splash

### Web playtest

- [ ] `npm run dev` loads ROM without error banner
- [ ] Canvas scales with window size
- [ ] Theater mode hides sidebar and fills the viewport
- [ ] Fullscreen toggle works on the canvas wrap
- [ ] Keyboard controls match GBA
- [ ] Command queue `right a start` enqueues and runs
- [ ] Typing in command field works (emulator input disabled while focused)
- [ ] Drag-drop `.gba` replaces ROM
- [ ] Soft reset reloads game

### Screen-flow scenarios

| ID | Flow | Expected |
|----|------|----------|
| M1 | Title → Start → Level 1 → Build → Start test → Win | HATT OK / PEEP |
| M2 | Title → Level 2 → weak bridge → Test | DELAY / ASHES |
| M3 | Build with no connections → Start | NO GO toast |
| M4 | Pause → Restart / Levels / Title | Correct screen transitions |
| M5 | Web theater mode + resize | Canvas scales without clipping |

### Docker / CapRover

- [ ] `docker build` succeeds after `sync_rom_to_web.sh`
- [ ] `curl -I http://localhost:8080/` shows COOP/COEP headers
- [ ] `/health` returns `ok`
- [ ] Game canvas renders (not permanently black)

## Browser automation note

Cloud-agent browser tests confirmed mGBA-WASM loads the ROM and the command queue fires `Start` successfully when the dev server provides cross-origin isolation headers.
