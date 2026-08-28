# morris-bridge-gba — Documentation Wiki

Knowledge base for **Thomas's Bridge** (ROM codename `thomas_bridge`): a Game Boy Advance bridge-building prototype built with [Butano](https://github.com/GValiente/butano).

## Quick links

| I want to… | Start here |
|------------|------------|
| Play the ROM right now | [Getting started](getting-started.md) |
| Understand the game | [Game design](game-design.md) |
| Optional creepy lore (PG, friends-only) | [Lore leak](lore-leak.md) |
| Full game + graphics roadmap | [Future expansion](future-expansion.md) |
| Session learnings & backlog | [Session knowledge](knowledge.md) |
| Build the GBA ROM | [Building the ROM](building-rom.md) |
| Run physics tests on PC | [Testing](testing.md) |
| Play in a browser | [Web playtest](web-playtest.md) |
| Hear the parody score | [Audio](audio.md) |
| Deploy for friends | [Deployment](deployment.md) |
| Set up CI/CD | [CI/CD pipeline](ci-cd.md) |
| Flash cart / hardware | [Hardware](hardware.md) |
| Fix something broken | [Troubleshooting](troubleshooting.md) |
| Git / repo setup | [Repository](repository.md) |

## Documentation map

```
docs/
├── README.md              ← You are here (wiki index)
├── getting-started.md     Play ROM, web, or Docker in 5 minutes
├── overview.md            Story, goals, legal disclaimer
├── architecture.md        Code modules and data flow
├── game-design.md         Modes, controls, win/lose
├── lore-leak.md           Optional PG copypasta backstory (friends)
├── future-expansion.md    Engines, progression, graphics roadmap
├── knowledge.md           Session learnings, pitfalls, backlog
├── physics.md             Fixed-point math and tension model
├── graphics.md            Sprites, palettes, asset pipeline
├── audio.md               Original parody score, SFX, Maxmod import
├── building-rom.md        devkitARM, Butano, Makefile, Docker
├── testing.md             Host unit tests and manual QA
├── web-playtest.md        mGBA-WASM browser emulator
├── deployment.md          Docker + CapRover for friends
├── ci-cd.md               GitHub Actions release workflow
├── hardware.md            GBA vs GB, EverDrive, flash carts
├── repository.md          Remotes, branches, morris-bridge-gba
├── scripts.md             Helper shell scripts
├── troubleshooting.md     Common failures
├── changelog.md           Project history and decisions
└── glossary.md            Terms used in this repo
```

## Repository

- **Name:** `morris-bridge-gba`
- **Origin:** `https://origin.cursor.com/git/hackmods/morris-bridge-gba.git`
- **ROM output:** `rom/thomas_bridge.gba` (~95 KB)
- **License:** zlib (same as Butano) — see root [README](../README.md)
