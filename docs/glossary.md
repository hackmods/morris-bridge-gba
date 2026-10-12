# Glossary

| Term | Definition |
|------|------------|
| **Butano** | Modern C++ high-level engine for GBA by GValiente |
| **bn::fixed** | Butano 20.12 fixed-point type (`1.0 = 4096`) |
| **Strut** / **link** | Member joining two nodes: timber (track) or rod (brace). An axial spring that snaps past its strength |
| **Node** / **joint** | Point in the bridge graph; anchored (bedrock, never moves) or player-placed (has weight, moves) |
| **Build mode** | Player places structure; `SimStatus::idle` |
| **Test mode** | Train simulation running |
| **Route** | Shortest climbable track path from start to end bank (Dijkstra by length) |
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
| **Beam length** | Longest timber the level's yard supplies (`level::Def::safe_length`); rods reach 1.5×, cable 2× |
| **Par** | Build cost (joints + timber + 2× rods) of the reference design; meeting it is 3★ |
| **Scheduled engine** | The engine the level is timetabled for; lighter engines clear it for 1★ only |
| **Rewind** | Leaving a test restores the build; links remember the worst force they carried |
