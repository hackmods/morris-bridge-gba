# Glossary

| Term | Definition |
|------|------------|
| **Butano** | Modern C++ high-level engine for GBA by GValiente |
| **bn::fixed** | Butano 20.12 fixed-point type (`1.0 = 4096`) |
| **Strut** | Edge connecting two nodes; has length, current tension, max tension |
| **Node** | Point in the bridge graph; may be anchored (bank) or player-placed |
| **Build mode** | Player places structure; `SimStatus::idle` |
| **Test mode** | Train simulation running |
| **BFS** | Breadth-first search used to find start→end path |
| **COOP / COEP** | HTTP headers required for `SharedArrayBuffer` (mGBA-WASM) |
| **GHCR** | GitHub Container Registry (`ghcr.io`) |
| **CapRover** | Self-hosted PaaS for Docker deploys |
| **thomas_bridge** | Project/ROM codename (`thomas_bridge.gba`) |
| **Thomas's Bridge** | Player-facing game title |
| **morris-bridge-gba** | Git repository name (historical) |
| **leo_bridge** | Retired early prototype codename |
| **TPP** | Twitch Plays Pokémon — inspiration for command queue UI |
| **EverDrive GBA** | GBA flash cartridge (not EverDrive GB) |
| **devkitARM** | Official GBA/homebrew toolchain from devkitPro |
| **grit** | Asset converter in Butano pipeline (BMP → GBA tiles) |
| **SAFE_LENGTH** | Reference span (48 px) for strut strength rating |
| **BASE_WEIGHT** | Train load constant in tension formula |
