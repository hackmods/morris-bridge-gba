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
| **Title** | Full splash artwork; Start / A → levels |
| **Level select** | Up/Down, A play, B back |
| **Build** | D-Pad, A nodes, Start test, Select pause |
| **Test** | Train runs; B cancel |

## Levels

1. Knapford Creek · 2. Gordon's Hill · 3. Vicarstown

## Controls

See [README](../README.md) and in-game HUD. Cursor snap radius: 10px.

## Audio

Original parody score (not the licensed TV theme): title march, yard idler, workshop build loop, full-steam crossing. Steam chuffs, peeps, and a win whistle. See [Audio](audio.md).

```bash
python3 tools/gen_graphics.py
```

Produces splash (128×128 hero art), sky, panels, Hatt, train, and HUD sprites.
