# Thomas's Bridge

**Thomas's Bridge** — a meme-forward GBA bridge-building game (Butano / C++20).

Help a cheeky blue tank engine cross Sodor gaps while **Sir Topham Hatt** watches for confusion and delay. Original pixel art — a playful Thomas homage, not a licensed product.

## Play now

| Method | Command / file |
|--------|-----------------|
| **ROM** | [`rom/thomas_bridge.gba`](rom/thomas_bridge.gba) → mGBA or EverDrive **GBA** |
| **Browser** | `./scripts/sync_rom_to_web.sh && cd web && npm install && npm run dev` |
| **Docker** | `./scripts/sync_rom_to_web.sh && docker build -t morris-bridge-gba . && docker run --rm -p 8080:80 morris-bridge-gba` |

## Features

- **Illustrated splash screen** — 128×128 pixel-art canyon scene with Thomas, broken bridge, and Hatt
- **Three levels** — Knapford Creek, Gordon's Hill, Vicarstown
- **Sir Topham Hatt** — stern approval or furious delay memes
- **Pokémon-style UI** — bordered panels, sky backdrop, animated water

## Documentation

See [`docs/README.md`](docs/README.md)

## Repository

Git repo: `morris-bridge-gba` (historical name). Game title: **Thomas's Bridge**.
