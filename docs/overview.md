# Project overview

## What is Thomas's Bridge?

A **Game Boy Advance** bridge-building puzzle game. The player places **joints** and welds timber and steel **links** between them to span a gap. The bridge is simulated as a real truss — it sags, carries load through its members, and snaps where it is overloaded — and a tank engine has to get across it.

**Resolution:** 240×160 (GBA native)  
**Engine:** [Butano](https://github.com/GValiente/butano) (modern C++20 GBA framework)  
**Math:** 100% fixed-point (`bn::fixed`, `bn::fixed_point`) — no floating-point on device

## Story (fiction)

**Thomas** (and friends) need your bridges across Sodor gaps while **Sir Topham Hatt** watches for confusion and delay. Original pixel art — a playful Thomas homage, **not** a licensed Thomas & Friends product.

## Design goals

1. **Mechanics prototype** — prove build mode + physics-based strut failure + train traversal
2. **Fixed-point discipline** — all physics uses Butano 20.12 fixed math
3. **Fast verification loop** — host unit tests, browser playtest, pre-built ROM in repo
4. **Shareable deploy** — Docker + CapRover so friends can play in a browser

## What is in scope (v1)

- Build mode: place nodes, connect struts, chain-building
- Test mode: fixed-point truss simulation ([Physics](physics.md)) — self-weight, moving axle loads, links snap in tension or compression
- Train takes the shortest climbable track route; grades steeper than 1 in 2 are not a route
- Eight levels, each a different structural problem (prop, hang, truss, strut, trestle, stay)
- Thomas choo-choo SFX (chuff, whistle, peep) plus original parody BGM via Maxmod
- Host physics tests (PC, no GBA toolchain)
- Web playtest (mGBA-WASM) with keyboard + command queue
- CI: ROM build → tests → Docker image → optional CapRover deploy

## Out of scope (future ideas)

- Twitch live chat integration (command queue simulates TPP without OAuth)
- Save states in browser
- Licensed Thomas & Friends recordings (original parody score only)
- Game Boy / GBC port (different CPU, no `bn::fixed` engine drop-in)
- Authentic licensed characters

## Codenames

| Name | Meaning |
|------|---------|
| `thomas_bridge` | Butano project / ROM filename (`thomas_bridge.gba`) |
| `morris-bridge-gba` | Git repository name (historical) |
| Thomas's Bridge | Player-facing title embedded in ROM header |
| `leo_bridge` | Retired early prototype codename — do not use in new paths |
