# Testing

## Host physics tests (recommended before every ROM deploy)

Compiles real [`physics.cpp`](../src/physics.cpp) and [`bridge_system.cpp`](../src/bridge_system.cpp) against a **20.12 `bn::fixed` mock** — no Butano, no emulator, no floating-point.

```bash
make -C tests
```

### What is tested

| Category | Examples |
|----------|----------|
| Math | `isqrt64`, `approx_sqrt`, 3-4-5 distance |
| Tension | Midspan, endpoints, zero-length edge case |
| Simulation | Two-span success, three-span success, overlong span failure, `no_path` |
| Rating | `max_tension_for_length` at 48 px, 96 px, and 176 px |
| Levels | Per-level seed counts, START/END positions, invalid index clamping |
| Build helpers | Preview statuses, undo chain, player node budget |

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

## CI integration

The **Release** workflow runs:

1. `make -C tests` in the `host-tests` job
2. `npm test` in the `web-tests` job
3. Docker image build after both pass

See [CI/CD](ci-cd.md).

## Manual QA checklist

### GBA / emulator

- [ ] Title shows **A NODE**, **START RUN**, and blinking **START**
- [ ] Level select shows track names without overlapping labels
- [ ] First build shows rotating hints, then **SEL PAUSE**
- [ ] D-Pad moves cursor; R speeds it up
- [ ] A places nodes and connects chain
- [ ] Start runs train; short bridge succeeds
- [ ] Long single span snaps; train falls
- [ ] No struts → "NO GO" / no path
- [ ] Select opens pause menu

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
