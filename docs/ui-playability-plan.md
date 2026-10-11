# UI & Playability Plan — Lessons + Fix Path

**Status:** Partly superseded (2026-10-11). The mechanics this plan assumed from PR #73 were replaced by the truss solver in [physics.md](physics.md); see the changelog. Still open from this plan: a hand playtest of Knapford with only on-screen hints (§4 exit criterion), menu clarity (§3), moving HUD chrome to a background layer (§5), and hand-authored art (§6 — note terrain is now generated from level data, see [graphics.md](graphics.md)).  
**Product rule:** Ship `rom/thomas_bridge.gba` (Butano). Web = mGBA-WASM playtest only. See [AGENTS.md](../AGENTS.md).  
**Related:** [Master plan](master-plan.md) · [Session goal](session-goal.md) · [Diagnosis (vision systems)](diagnosis-vision.md) · [Graphics](graphics.md) · [Game design](game-design.md)

Paste the **GOAL** block at the bottom into a new Cursor / Cloud Agent session to continue.

---

## 1. Lessons learnt (do not re-litigate)

### 1.1 The mockups are the *target*, not a pixel dump

Gemini / concept frames for “Sodor Bridge Builder” show:

- Full header (title, level name, £ budget, Thomas target)
- Right-side Wood / Iron / Stone / Cable strip with icons + costs
- Readable bottom control bar (A DRAW · B DELETE · L/R CYCLE)
- Recognizable Thomas + coaches, rocky ravine, grid in EDIT
- Green → yellow → orange → red stress heatmap while the train crosses
- Hatt portrait + full “confusion and delay” RESULT copy

Real GBA constraints (240×160, ≤16 colors/sprite, ≤10 sprites/scanline, no float/heap in sim) mean we **aim for that language**, not a 1:1 blit of AI mockups.

### 1.2 Systems ≠ shippable game

PR #73 / vision work landed real **mechanics**:

| Done | Still weak |
|------|------------|
| `MaterialId` + £ costs + budget purse | Looks like placeholder `gen_graphics.py` art |
| 8×8 snap, load-path truss, cascade, 4-band stress | Stress barely *reads* without beam art |
| EDIT / SIM / RESULT aliases + HUD labels | HUD is tiny floating labels, not mockup chrome |
| A / B / L/R / START controls | Build feel is fiddly; D-pad macros miss under WASM |
| Host playthroughs green | Menus feel broken / unclear to players |

**Lesson:** Another physics refactor will not fix “looks like shit” or “doesn’t play well.” Next work is **feel + authored art + readable chrome**.

### 1.3 OAM / HUD mistakes

- Bottom chrome at ground Y fought scenery (GBA ≤10 sprites/line) → labels vanished.
- Fix direction: sky-row chrome (`HUD_STRIP_Y`), right-edge material (`HUD_MAT_X`), `Z_HUD`, prefer **BG bars** for permanent chrome long-term.
- White scaled panels hid white digits — don’t put opaque white under digit sprites.

### 1.4 Playtest / input lessons

- Wall-clock `holdButton(ms)` ≠ reliable GBA frames under mGBA-WASM; 8×8 snap makes D-pad macros miss nodes.
- **L+R+START** loads host `level_solutions` reference then enters SIM — QA assist only, not a substitute for a teachable build loop.
- “Menu didn’t work” is often: unclear Start vs A, tiny level-select cards, Select=pause surprising players, or build never making a path so START feels dead.

### 1.5 What not to do

- Do **not** replace the GBA game with a browser Poly Bridge / canvas workshop.
- Do **not** demote the ROM to “legacy.”
- Do **not** overwrite authored BMPs with `gen_graphics.py` once art lands (validator-only).
- Do **not** restart dense FEM; keep documented load-path truss ([physics.md](physics.md)).

---

## 2. Goals (ordered)

Same pillars as the mockup conversation, scoped for GBA:

| # | Goal | Player feels |
|---|------|--------------|
| A | **Menus work** | Title → level → engine → build is obvious; pause/resume never traps |
| B | **Build plays well** | Cursor snaps, A draws, B deletes, undo works, START only when a track path exists |
| C | **Chrome matches vision language** | Budget £, material strip, control legend, SIM stress %, RESULT Hatt |
| D | **Art escapes placeholders** | Authored train, beams, nodes, BG theme on **Knapford first** |
| E | **SIM/RESULT read** | Heatmap readable; fail/win copy + Hatt portrait; juice (shake/dust) |

Vertical slice rule: **Knapford Creek** must feel good end-to-end before art-spreading to Gordon / Vicarstown.

---

## 3. Plan — Phase 0: Diagnose menus (½ session)

Reproduce on mGBA-WASM and note exact break:

1. Title: Start / A → level select?
2. Level select: Up/Down, A → engine, B → title?
3. Engine select: A → build (or intro), B → levels?
4. Build: Select → pause → Resume / Levels / Title?
5. START with no path → NO GO and stay in EDIT?

**Deliverable:** short bullet list in this doc §7 “Known menu bugs” (fill as found) + fix commits.

**Likely fixes (code already exists — polish, don’t rewrite):**

- Larger / clearer level-select (`track_card` or BG panel + name label)
- Title hints that match real controls (A DRAW / START, not stale HOLD A only)
- Pause option labels always visible; never leave `_screen` inconsistent
- Soft-lock audit: every screen has a B or Select escape

---

## 4. Plan — Phase 1: Playability (feel first, no new art)

Ship these before pretty pixels:

1. **Cursor & snap**
   - Snap feedback (ring / ghost always readable)
   - Widen hover radius slightly if 8×8 feels sticky
   - Hold-A chain weld stays; show preview stress color

2. **Tools**
   - A = draw / confirm (current)
   - B = delete strut under cursor else node (current) — toast always fires
   - L/R = cycle **allowed** materials only; show cost next to name when possible
   - Reliable **undo** (strut then node) — restore L-as-undo *or* dedicate a pause action if L stays cycle

3. **START gate**
   - If no track path: NO GO + stay EDIT (already) — make toast louder / longer
   - If path OK: legend shows START (already)

4. **SIM clarity**
   - Top: `RUN!` + stress % (already STRESS digits) — keep digits big
   - Cancel with B returns to EDIT without soft-lock
   - Fail: cascade readable; RESULT after short settle (not instant flash)

5. **Teach the loop**
   - First-build hints: A DRAW → B DEL → L/R CY → START (already cycling) — keep on sky row, not ground OAM
   - Optional: one-time “draw bank to bank” arrow

6. **QA**
   - Keep L+R+START reference fill for CI / demos
   - Add Playwright that uses it for SIM evidence; keep host `make -C tests` as physics SoT

**Exit criteria:** Fresh player (or agent) can clear Knapford with only on-screen hints, no L+R+START.

---

## 5. Plan — Phase 2: Vision chrome (UI layout)

Implement mockup *language* inside GBA limits:

| Element | Approach |
|---------|----------|
| Top bar | Prefer **BG window / solid bar** (not OAM over ground): level short name, BUDGT £, target load |
| Right material strip | 4 icon slots (or 2+cycle) at `HUD_MAT_X`; highlight selected; £ under icon |
| Bottom legend | BG strip or sky-row labels: A DRAW · B DEL · L/R · START |
| EDIT grid | Optional faint BG dots or cursor-aligned ghost cells |
| SIM | Hide grid; show RUN + stress %; heatmap on struts |
| RESULT | Panel + Hatt bust + CONFUSION/DELAY or HATT OK / USEFUL; retry / next |

Reuse constants in `include/game_ui.h` (`HUD_TOP_Y`, `HUD_STRIP_Y`, `HUD_MAT_X`, `Z_HUD`). Prefer raising chrome into BG to free OAM for train/links.

**Exit criteria:** Screenshot parity *in layout* with mockups on Knapford EDIT / SIM / RESULT (art can still be placeholder).

---

## 6. Plan — Phase 3: Authored art vertical slice (Knapford)

Escape `gen_graphics.py` look:

1. Author in Aseprite (≤16 colors, index 0 clear):  
   - 32×32 Thomas (+ optional coaches)  
   - Track / wood / iron / cable / stone strut frames  
   - Node + anchor  
   - Hatt stern / furious / peep  
   - Knapford BG (sky / hills / water / banks)
2. Drop BMPs into `graphics/`; JSON basenames unchanged for Butano.
3. `validate_graphics.py` in CI; switch `gen_graphics.py` to **--check only** for those assets.
4. Stress palette rows on strut sheet (green / yellow / red / snap).
5. Juice: shake + dust on snap (hooks exist).

Then extend art to Gordon’s Hill / Vicarstown themes ([master-plan §7](master-plan.md)).

**Exit criteria:** Side-by-side “before/after” on Knapford; friends don’t say “placeholder” first.

---

## 7. Known menu / play bugs

*(Next session: fill after repro. Starters from 2026-09-24 review:)*

- [ ] Confirm title → level → engine → build on web playtest with on-screen pad
- [ ] Level select readability (`track_card` too small / unclear?)
- [ ] Pause (Select) discoverability
- [ ] Build path rarely completes without reference cheat under human D-pad
- [ ] RESULT fail copy still short vs mockup paragraph
- [ ] Material strip shows one label, not four icons

---

## 8. Non-goals (this track)

- Browser-native bridge builder as product
- Full dense FEM matrix
- All 8 levels re-arted before Knapford feels good
- Licensed Thomas assets / theme music

---

## 9. Suggested session GOAL (copy-paste)

```
GOAL

Read docs/ui-playability-plan.md and AGENTS.md first. Product is rom/thomas_bridge.gba only.

Execute in order:
0) Reproduce and fix menu / soft-lock issues (Phase 0); document in §7.
1) Phase 1 playability on Knapford — feel, START gate, undo, hints — until a fresh run can win without L+R+START.
2) Phase 2 vision chrome — BG top/bottom bars, right material strip, SIM/RESULT layout.
3) Phase 3 authored art vertical slice for Knapford (train, beams, nodes, BG); validate_graphics; no gen_graphics overwrite of authored BMPs.

Keep make -C tests and web unit tests green. Commit/push often. Do not replace the GBA game with a web workshop.

Definition of done for this track:
- Menus never trap; Knapford clearable by hand
- EDIT/SIM/RESULT chrome matches vision layout language
- Knapford no longer reads as pure gen_graphics placeholder
- Demo screenshots: EDIT chrome, SIM heatmap+train, RESULT Hatt
```

---

## 10. Quick commands

```bash
make -C tests
cd web && npm test
./scripts/sync_rom_to_web.sh   # after CI ROM download
# Vision / feel captures:
cd web && WALKTHROUGH_DIR=/opt/cursor/artifacts \
  npx playwright test e2e/capture-vision-loop.spec.ts
```

QA assist (not for “player can play” exit criteria): **L+R+START** = load reference + SIM.
