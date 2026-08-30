# Game design

## Thomas's Bridge

A meme-forward bridge-building puzzle for Game Boy Advance. Help Thomas cross Sodor gaps while **Sir Topham Hatt** watches — no confusion, no delay.

Original pixel art and parody dialogue; not a licensed Thomas & Friends product.

## Naming

Launch title: **Thomas's Bridge**. (Tank Bridge is too generic; Morris was the working codename.)

## Meme flavour

| Moment | Line / beat |
|--------|-------------|
| Splash | Illustrated canyon + THOMAS'S BRIDGE logo |
| Win | HATT OK! + PEEP! (rare: CHUFF!, TOOT!, banner **PEEP PEEP**) |
| Snap / fall | DELAY! + ASHES! (rare: OOPS!, WAIT!, banner **LISTEN**) |
| No path | NO GO |

Optional **PG lore** for friends: [Lore leak](lore-leak.md) (creepy copypasta backstory; safe on the surface for kids).

## Screens

| Screen | Controls |
|--------|----------|
| **Title** | Full splash artwork; hints for **HOLD A** and **A WELD**; Start / A → levels |
| **Level select** | Up/Down (locked tracks skipped), A → engine pick, B back — track name shown in menu row |
| **Engine select** | Up/Down (locked engines skipped), weight shown; A start, B back |
| **Build** | Hold **A** and drag to weld; tap A to join joints; Start tests; Select pause; B delete; L undo |
| **Test** | Train runs; B cancel |

## Onboarding

The ROM teaches controls directly:

- **Title:** static hint labels (**HOLD A**, **A WELD**) above the dialog panel
- **First build session:** rotating hints (`HOLD A` → `A WELD` → `PAUSE`) for a few seconds, dismissible with any button
- **Build HUD:** **A WELD** while a joint is selected, **START** once a road links both banks

## Engines and unlocks

| Engine | Load | Unlock |
|--------|------|--------|
| Thomas | 4 | Start |
| Percy | 3 | First clear — Knapford Creek |
| Edward | 5 | First clear — Gordon's Hill |
| James | 6 | First clear — Vicarstown |
| Gordon | 10 | Every track cleared |

Heavier engines snap struts sooner. Build-mode weak-strut warnings use the selected engine's load.

Code: [`include/engine.h`](../include/engine.h), [`include/progress.h`](../include/progress.h).

## Levels

1. Knapford Creek · 2. Gordon's Hill · 3. Vicarstown · 4. Tidmouth Yard
5. Wellsworth Gap · 6. Crovan's Gate · 7. Callan Canal · 8. Sodor Summit

## Controls

See [README](../README.md) and in-game HUD. Cursor snap radius: 10px.

## Roadmap

Engine unlocks, weight-based difficulty, and graphics upgrade plan: [Future expansion](future-expansion.md).

## Audio

Original parody score (not the licensed TV theme): title march, yard idler, workshop build loop, full-steam crossing. Steam chuffs, peeps, and a win whistle. See [Audio](audio.md).

## Graphics

```bash
python3 tools/gen_graphics.py
python3 tools/validate_graphics.py
```

Produces splash (128×128 hero art), sky, panels, Hatt, train, and HUD sprites.
