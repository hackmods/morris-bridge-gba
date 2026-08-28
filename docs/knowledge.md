# Session knowledge & backlog

Living notes from Cloud Agent sessions: what shipped, what broke, and what to pick up next. Complements [Changelog](changelog.md) (history) and [Future expansion](future-expansion.md) (roadmap).

---

## PR merge policy (read first)

**Do not stack open PRs.** Merge to `main` as soon as CI is green.

| Rule | Detail |
|------|--------|
| **One active code PR** | Superseded branches get closed, not left open alongside replacements |
| **Merge order** | Docs/plan PR first if separate, then the single consolidated code PR |
| **Mark ready** | Flip draft → ready when host tests + web tests + E2E pass (Docker deploy failure is non-blocking for game code) |
| **Delete branch** | After merge, delete the remote `cursor/*` branch |

**Current queue (2026-08-28):**

1. Merge **#14** (`master-plan` docs) — no ROM impact  
2. Merge **#15** (`engine-v2-link-role`) — **supersedes #13**; close #13 without merging  
3. Delete `cursor/fix-levels-backdrop-playthrough-7b6f` after #15 lands  

Agents: after pushing a fix, **update the existing PR or merge it in the same session** — do not open a third PR for the same workstream.

---

## Current state (2026-08-28)

| Item | Value |
|------|-------|
| **Base branch** | `main` @ `9d60af2` (Playwright E2E merged) |
| **Game title** | Thomas's Bridge |
| **ROM** | `rom/thomas_bridge.gba` (~264 KB) |
| **Repo** | `morris-bridge-gba` (historical name; keep for URLs/remotes) |

### Shipped this session (merged PRs)

| PR | Summary |
|----|---------|
| **#8** | Full expansion: five engines, unlock progression, SRAM save, three levels, 32×32 art, parallax, juice |
| **#6** | Audio refactor: `tools/gen_audio.py`, parody score/SFX, `game_constants.h`, ground snap, structural reinforcement |
| **#5** | UX polish: onboarding hints (`A NOD`, `START`, `PAUSE`), web layout tests, wider backdrop |
| **#7** | PG lore (`docs/lore-leak.md`, rare HUD alternates) — content absorbed into expansion |
| **#9** | Distinct hand-tuned train sprites per engine in `tools/gen_graphics.py` |
| **#10** | Playwright E2E for web playtest (12 tests, CI `web-e2e` job) |

All `cursor/*` feature branches were merged and remote branches deleted after integration.

---

## Test commands (local parity with CI)

```bash
make -C tests                          # host C++ physics + audio check
cd web && npm test                     # Vitest (11 tests)
./scripts/sync_rom_to_web.sh            # required before E2E / Docker
cd web && npx playwright install chromium && npm run test:e2e
```

---

## Known pitfalls (do not re-learn)

### Web / E2E

| Pitfall | Fix |
|---------|-----|
| Playwright `webServer` 120s timeout | Vite must bind **`127.0.0.1`**, not default `localhost` (IPv6-only on some runners). See [`web/vite.config.ts`](../web/vite.config.ts). |
| Headless canvas “not rendering” via `toDataURL()` or frame counter | mGBA-WASM `videoFrameEndedCallback` stays at 0 in headless Chromium. Use **Playwright canvas screenshots** and pixel-diff helpers in [`web/e2e/helpers.ts`](../web/e2e/helpers.ts). |
| `assertCanvasIsRendering` reloads page | Never call `page.goto()` inside mid-test rendering checks — breaks theater-mode toggle tests. |
| Theater mode hides header | `#btn-theater` is not visible in theater mode; E2E only enters theater, does not exit via UI click. |
| Keyboard Enter → Start flaky in headless | Use `window.__TEST__.tapButton('Start')` from [`web/src/test-hooks.ts`](../web/src/test-hooks.ts). |
| E2E without ROM | Run `./scripts/sync_rom_to_web.sh` first — copies `rom/thomas_bridge.gba` → `web/public/rom/`. |

### Merge / architecture

| Pitfall | Resolution used |
|---------|-----------------|
| Expansion + audio refactor conflict | Keep `level` module, engine/progress/save; merge onboarding hint label indices with lore indices 11–16 |
| Coach weight vs structural reinforcement | Combined: coach weight physics + `effective_max_tension` + triangular bracing from PR #6 |
| Level seed Y coordinates | Align seeds to `game::TRACK_Y` (52 px) in [`src/level.cpp`](../src/level.cpp) |

### Docs drift

Several wiki pages still referenced **`leo_bridge.gba`** / Ryan's Bridge from the early prototype. Canonical names are **`thomas_bridge.gba`** / **Thomas's Bridge**. Search docs for `leo_bridge` before assuming paths.

---

## Outstanding work

### High value

- [ ] **Verify CI `web-e2e` on `main`** after merge — needs ROM artifact + Playwright Chromium on GitHub runners.
- [ ] **Doc rename sweep** — any remaining `leo_bridge` / Ryan's Bridge references outside this wiki pass.
- [ ] **Manual screen-flow QA (M1–M5)** — listed in [Testing](testing.md#screen-flow-scenarios); not yet automated.

### Medium

- [ ] **Per-level BG tilesets** — parallax is sprite-based today; move scenery to regular BGs per [Future expansion §5B](future-expansion.md#phase-b--background-layers-biggest-immersion-win).
- [ ] **Aseprite-native art pass** — replace procedural BMPs; add `tools/validate_graphics.py`.
- [ ] **Extended engine roster** — Toby, Diesel, Emily slots reserved in design doc only.
- [ ] **CapRover post-deploy smoke** — curl COOP/COEP + canvas boot script (no Playwright in prod container today).

### Low / optional

- [ ] **ROM build without Docker locally** — document devkitPro path more prominently; Docker remains CI source of truth.
- [ ] **`BN_TEST_HOOKS` in ROM** — debug frame counters on device (web has `window.__TEST__` only).
- [ ] **Mode 3 splash** — single full-screen bitmap title instead of sprite composite.

---

## QoL & stretch goals

### Testing

| Goal | Notes |
|------|-------|
| Golden screenshot E2E | Baseline PNG per spec; fail on visual drift. Start with `rom-boot` + title screen. |
| M1–M5 screen-flow automation | Extend [`web/e2e/gameplay.spec.ts`](../web/e2e/gameplay.spec.ts) with win/lose/pause paths via `__TEST__` hooks. |
| `preserveDrawingBuffer` | Optional WebGL/canvas flag if future tests need in-page pixel reads instead of screenshots. |
| Playwright in CapRover smoke | Hit deployed URL, one screenshot assertion after ROM load. |

### Gameplay / content

| Goal | Notes |
|------|-------|
| Per-level wind / lateral force | `LevelDef::wind_bonus` stub exists in roadmap |
| Star ratings + replay incentives | Bitmask in progress save |
| Coach follow cars | 16×16 sprites parented to train path |
| Diesel “lying” stat screen | Lore/gameplay gag from future roster |

### Web playtest

| Goal | Notes |
|------|-------|
| Save states in browser | Out of scope v1; mGBA-WASM may support later |
| Twitch chat integration | Command queue already simulates TPP without OAuth |

### Dev experience

| Goal | Notes |
|------|-------|
| `npm run test:all` | Single script: Vitest + E2E with ROM sync guard |
| Pre-commit hook | `make -C tests` + `npm test` for touched paths |
| Faster CI | Cache Butano clone + Docker layers (partially done via GHA cache) |

---

## File map (session touchpoints)

```
tools/gen_graphics.py     # five 32×32 engine templates, portraits, splash
tools/gen_audio.py          # parody BGM/SFX generation
web/e2e/*.spec.ts           # Playwright E2E (12 tests)
web/src/test-hooks.ts       # dev-only window.__TEST__ (omitted in production)
web/vite.config.ts          # host: 127.0.0.1 for Playwright
.github/workflows/release.yml  # build-rom, host-tests, web-tests, web-e2e, build-image, deploy
include/engine.h            # five-engine profiles
include/progress.h          # unlocks + SRAM
src/level.cpp               # level seeds, TRACK_Y alignment
docs/knowledge.md           # ← this file
```

---

## Prebuilt ROM policy

**Rule:** After any major refactor, run `./scripts/build_rom.sh` and commit `rom/thomas_bridge.gba`.

| Trigger | Examples |
|---------|----------|
| Gameplay / sim | `src/`, `include/`, level seeds, engine profiles |
| Assets | `graphics/`, `tools/gen_graphics.py`, `tools/gen_audio.py` |
| Build | Makefile flags, Butano version, audio backend |
| Integration | Merging expansion/audio/graphics PRs into `main` |

Skip for docs-only or web-only changes that do not alter the GBA binary.

CI always builds a fresh ROM (`build-rom` job runs `gen_graphics.py` + `gen_audio.py` + Docker `make`). The committed copy is for friends who download the repo without Docker.

---

## Where to start next session

**Full ship:** Use [Session goal](session-goal.md) — copy the GOAL block into a new Cloud Agent session. Read [Master plan](master-plan.md) first (Engine v2 architecture, phases 1–7, AV spec).

**Incremental:** Run full local test suite (commands above), then pick from **Outstanding work** below.

1. Run full local test suite (commands above).
2. Check GitHub Actions **Release** workflow on latest `main` — all jobs green.
3. For v2 engine work: start **Phase 1** in [master-plan.md](master-plan.md) (LinkRole + track-only path + Gordon's Hill vertical slice).
4. For polish-only: pick one item from **Outstanding work → High value** or **QoL**.
