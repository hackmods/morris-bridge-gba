# Project overview

## What is Ryan's Bridge?

A **Game Boy Advance** demake prototype of a bridge-building puzzle game. The player places structural **nodes** and connects them with **struts** to span a canyon. A small tank engine must cross before struts snap under tension.

**Resolution:** 240×160 (GBA native)  
**Engine:** [Butano](https://github.com/GValiente/butano) (modern C++20 GBA framework)  
**Math:** 100% fixed-point (`bn::fixed`, `bn::fixed_point`) — no floating-point on device

## Story (fiction)

**Leo** is an autistic boy whose special interest is structural engineering. He helps save a train by building bridges across gaps. This is original fiction and placeholder art — **not** a licensed Thomas & Friends product.

## Design goals

1. **Mechanics prototype** — prove build mode + physics-based strut failure + train traversal
2. **Fixed-point discipline** — all physics uses Butano 20.12 fixed math
3. **Fast verification loop** — host unit tests, browser playtest, pre-built ROM in repo
4. **Shareable deploy** — Docker + CapRover so friends can play in a browser

## What is in scope (v1)

- Build mode: place nodes, connect struts, chain-building
- Test mode: train follows BFS path; tension breaks long spans
- Structural reinforcement (joint redundancy + triangular bracing)
- Locomotive engine physics (heavier first span, acceleration ramp)
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
| `leo_bridge` | Butano project / ROM filename (`leo_bridge.gba`) |
| `morris-bridge-gba` | Git repository name |
| Ryan's Bridge | Player-facing title embedded in ROM header |
