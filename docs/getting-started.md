# Getting started

Three ways to run Ryan's Bridge, fastest first.

## 1. Browser playtest (no GBA hardware)

```bash
./scripts/sync_rom_to_web.sh
cd web && npm install && npm run dev
```

Open **http://localhost:4317**. Use arrow keys + Z/X or the command queue (`right a start`).

See [Web playtest](web-playtest.md) for full details.

## 2. Pre-built ROM (emulator or flash cart)

The repo ships **`rom/leo_bridge.gba`**.

- **Emulator:** drag into mGBA or VisualBoyAdvance-M
- **EverDrive GBA:** copy to the `GBA` folder on your SD card

See [Hardware](hardware.md) for compatibility notes.

## 3. Docker (production-like web deploy)

```bash
./scripts/sync_rom_to_web.sh
docker build -t morris-bridge-gba:local .
docker run --rm -p 8080:80 morris-bridge-gba:local
```

Open **http://localhost:8080**.

## 4. Build ROM from source

Requires devkitARM + Butano, or Docker:

```bash
./scripts/build_rom.sh
```

Output: `leo_bridge.gba` (also copied to `rom/` and `web/public/rom/`).

See [Building the ROM](building-rom.md).

## Verify physics without an emulator

```bash
make -C tests
```

All assertions should pass. See [Testing](testing.md).
