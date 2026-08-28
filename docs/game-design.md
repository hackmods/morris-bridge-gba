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
| Win | HATT OK! + PEEP! |
| Snap / fall | DELAY! + ASHES! |
| No path | NO GO |

## Screens

| Screen | Controls |
|--------|----------|
| **Title** | Full splash artwork; hints for **A NOD** and **START**; Start / A → levels |
| **Level select** | Up/Down, A play, B back — track name shown in menu row |
| **Build** | D-Pad, A nodes, Start test, Select pause, B delete, L undo |
| **Test** | Train runs; B cancel |

## Onboarding

The ROM now teaches controls directly:

- **Title:** static hint labels (**A NOD**, **START**) above the dialog panel
- **First build session:** rotating hints (`A NOD` → `START` → `PAUSE`) for a few seconds, dismissible with any button
- **Build HUD:** persistent **PAUSE** reminder after the intro cycle

## Levels

1. Knapford Creek · 2. Gordon's Hill · 3. Vicarstown

## Controls

See [README](../README.md) and in-game HUD. Cursor snap radius: 10px.

## Graphics

```bash
python3 tools/gen_graphics.py
```

Produces splash (128×128 hero art), sky, panels, Hatt, train, and HUD sprites.
