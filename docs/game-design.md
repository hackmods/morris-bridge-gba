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
| **Title** | Full splash artwork; Start / A → levels |
| **Level select** | Up/Down, A play, B back |
| **Build** | D-Pad, A nodes, Start test, Select pause |
| **Test** | Train runs; B cancel |

## Levels

1. Knapford Creek · 2. Gordon's Hill · 3. Vicarstown

## Controls

See [README](../README.md) and in-game HUD. Cursor snap radius: 10px.

## Roadmap

Engine unlocks, weight-based difficulty, and graphics upgrade plan: [Future expansion](future-expansion.md).

Starter types: [`include/engine.h`](../include/engine.h), [`include/progress.h`](../include/progress.h).

## Graphics

```bash
python3 tools/gen_graphics.py
```

Produces splash (128×128 hero art), sky, panels, Hatt, train, and HUD sprites.
