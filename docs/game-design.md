# Game design

## Morris Bridge

A meme-forward bridge-building puzzle for Game Boy Advance. Help a cheeky blue tank engine cross Sodor gaps while **Sir Topham Hatt** watches — no confusion, no delay.

Original pixel art and parody dialogue; not a licensed Thomas & Friends product.

## Meme flavour

| Moment | Line / beat |
|--------|-------------|
| Title | "REALLY USEFUL!" |
| Win | "HATT OK!" + PEEP! |
| Snap / fall | "DELAY!" + ASHES! |
| No path | NO GO |
| All levels done | USEFUL! |

Sir Topham Hatt appears stern on menus and **furious** when you cause confusion and delay.

## Screens & navigation

| Screen | Enter | Controls |
|--------|-------|----------|
| **Title** | Boot | Start / A → level select |
| **Level select** | Title / pause | Up/Down (repeat), A/Start play, B → title |
| **Build** | Level select | D-Pad cursor, A nodes/struts, Start test, Select pause |
| **Test** | Start in build | Train runs; B cancel; success → Hatt approval |
| **Fail** | Strut snaps | B → rebuild |
| **Pause** | Select in build | Up/Down, A confirm, B resume |

## Levels

| # | Name | Vibe |
|---|------|------|
| 1 | Knapford Creek | Tutorial — generous anchors |
| 2 | Gordon's Hill | Wider span, higher banks |
| 3 | Vicarstown | Widest gap — delay incoming if you slack |

## Controls (GBA)

| Button | Build mode | Test mode |
|--------|------------|-----------|
| D-Pad | Move crosshair (magnetic snap to nodes) | — |
| R (hold) | Faster cursor | — |
| A | Place / connect / deselect same node | — |
| B | Cancel selection | Cancel test |
| Start | Run simulation | Cancel test |
| Select | Pause menu | — |

### Feel improvements

- Faster base cursor speed with diagonal normalisation
- Larger snap radius (10 px) — cursor locks to nearby nodes
- D-Pad repeat on menus
- Cursor starts beside the left bank each level
- Train flips to face travel direction

## Win / lose

| Outcome | Condition | HUD |
|---------|-----------|-----|
| Success | Train reaches end | USEFUL / PEEP! |
| No path | Disconnected graph | NO GO |
| Failure | Strut snaps | ASHES! + DELAY! banner |

## Graphics tool

```bash
python3 tools/gen_graphics.py
```

Generates Pokémon-style UI panels, sky backdrop, Sir Topham Hatt sprites, Thomas engine, and HUD labels.
