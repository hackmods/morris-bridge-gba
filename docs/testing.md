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
| Simulation | Two-span success, overlong span failure, `no_path` |
| Rating | `max_tension_for_length` at 48 px and long spans |

### Mock layer

[`tests/host_bn/`](../tests/host_bn/) provides minimal stand-ins:

- `bn_fixed.h` — 20.12 arithmetic
- `bn_fixed_point.h`
- `bn_vector.h`
- `bn_optional.h`

## CI integration

The **Release** workflow runs `make -C tests` in the `host-tests` job before building the Docker image. See [CI/CD](ci-cd.md).

## Manual QA checklist

### GBA / emulator

- [ ] Build screen loads with anchored bank nodes
- [ ] D-Pad moves cursor; R speeds it up
- [ ] A places nodes and connects chain
- [ ] Start runs train; short bridge succeeds
- [ ] Long single span snaps; train falls
- [ ] No struts → "NO GO" / no path
- [ ] Select resets level

### Web playtest

- [ ] `npm run dev` loads ROM without error banner
- [ ] Keyboard controls match GBA
- [ ] Command queue `right a start` enqueues and runs
- [ ] Typing in command field works (emulator input disabled while focused)
- [ ] Drag-drop `.gba` replaces ROM
- [ ] Soft reset reloads game

### Docker / CapRover

- [ ] `docker build` succeeds after `sync_rom_to_web.sh`
- [ ] `curl -I http://localhost:8080/` shows COOP/COEP headers
- [ ] `/health` returns `ok`
- [ ] Game canvas renders (not permanently black)

## Browser automation note

Cloud-agent browser tests confirmed mGBA-WASM loads `leo_bridge.gba` and the command queue fires `Start` successfully when the dev server provides cross-origin isolation headers.
