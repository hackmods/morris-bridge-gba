# Hardware compatibility

## Supported platforms

| Platform | Works? | Notes |
|----------|--------|-------|
| Game Boy Advance / GBA SP | **Yes** | Native target |
| Nintendo DS (GBA slot) | **Yes** | On models with a GBA cartridge slot |
| EverDrive GBA / EZ-Flash Omega | **Yes** | Copy `thomas_bridge.gba` to SD `GBA/` folder |
| mGBA, VBA-M (emulator) | **Yes** | Drag-and-drop ROM |
| Browser (web playtest) | **Yes** | See [Web playtest](web-playtest.md) |

## NOT supported

| Platform | Why |
|----------|-----|
| Original Game Boy (DMG) | Different CPU (SM83), no ARM |
| Game Boy Color | Different architecture |
| **EverDrive GB** | GB/GBC flash cart — wrong system |
| Analogue Pocket (GB mode) | GBA ROM required for GB mode adapters that only do GB |

This project outputs a **`.gba` file** (32-bit ARM7TDMI). It cannot run on 8-bit Game Boy hardware without a full port.

## Flashing workflow (EverDrive GBA)

1. Build or download `rom/thomas_bridge.gba`
2. Copy to the **`GBA`** folder on the EverDrive SD card
3. Insert cart into a GBA / GBA SP
4. Launch from the EverDrive menu

## Verification order

Recommended before hardware testing:

1. `make -C tests` — physics on PC
2. Web playtest or mGBA desktop — full game loop
3. EverDrive / hardware — final confirmation (timing, input feel)

## ROM details

- Size: ~330 KB (338,236 bytes as of 2026-08-29 release)
- Title: `THOMASBRIDGE`
- Code: `THOM01`
- Resolution: 240×160 @ 60 Hz target

Ship verification: [ship-qa.md](ship-qa.md)
