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
| Win | HATT OK! + a line that depends on how you won (see [Lore lines](#lore-lines)) |
| Snap / fall | DELAY! + a line that depends on how you lost |
| No route | NO GO |

Optional **PG lore** for friends: [Lore leak](lore-leak.md) (creepy copypasta backstory; safe on the surface for kids).

## Screens

| Screen | Controls |
|--------|----------|
| **Title** | Full splash artwork; hints for **HOLD A** and **A WELD**; Start / A → levels |
| **Level select** | Up/Down (locked tracks skipped), A → engine pick, B back — track name shown in menu row |
| **Engine select** | Up/Down (locked engines skipped), weight shown; A start, B back |
| **Build** | **D-pad** cursor · **A** tap join / **hold-A drag** chain-weld · **B** undo last node · **L** undo last strut · **R** timber / rod · **Select** pause · **Start** send the train · HUD **cost vs par** |
| **Test** | Train runs; B cancel |

## Onboarding

The ROM teaches controls directly:

- **Title:** static hint labels (**HOLD A**, **A WELD**) above the dialog panel
- **First build session:** rotating hints (`HOLD A` → `A WELD` → `PAUSE`) for a few seconds, dismissible with any button
- **Material palette:** icons at the top right show every material this track allows, with a ring on the one in hand and its name (**WOOD**, **ROD**, **CABLE**) and price underneath. **R** cycles through them.
- **Build HUD:** **A WELD** while a joint is selected, **START** once a road links both banks; digits show **cost vs par** (3★ if cost ≤ par). Point past the end of a beam and the weld ghost stops where the joint will land: **A** plants it there, at full reach. (Only a weld between two existing joints that are too far apart is refused: **LONG!**) A finished deck with a beam too steep to climb shows **STEEP!** and that beam blinks red. After a test, links that carried ≥ 75 % of their strength (or snapped) glow **hot**.
- **Hold A + drag:** plants joints every 24 px so a span can be chain-welded in one stroke; tap A still joins one joint at a time

## How a bridge works

Test mode is a structural simulation ([Physics](physics.md)), not a rating table.

- **Anchors** (banks, rocks, crags, pylons) are the only solid ground.
- **Joints** you place have weight and fall unless something holds them.
- **Timber** carries the rails. It is cheap (1), short, and snaps at 8.
- **Rod** is a brace only. It costs 2, reaches 1.5× as far, and snaps at 12.
- **Cable** costs 1, reaches twice as far, and holds 12 — but only in tension.
  Pushed shorter than its length it hangs slack and carries nothing. Hang with
  cable; prop with timber or rod.
- Links snap in **tension or compression**. A snapped link dumps its load on the
  neighbours.
- The bridge **settles** under its own weight before the train leaves.
- The train takes the **shortest track route**; track steeper than 2 in 3 is not
  a route, so timber can be a post without the engine trying to drive down it.
- A train that sags 20 px below the line is in the water.
- The run is won when the **whole train** is across — engine and every coach.
- **Colours:** a link turns amber at half its strength and red at four-fifths.
  During a run the HUD shows **LOAD** (worst link now, %) and **PEAK** (worst so far).
- **The purse:** a bridge may cost at most twice par. Past that
  the yard will not weld: **SPENT**. The right-hand number reads **PAR** until you
  are over it, then **MAX**.
- **B** on a joint deletes the joint; **B** on a beam deletes just that beam. **L** undoes the newest weld and keeps unwinding if you keep pressing.
- Hold **A** during a run for double speed.
- On a gradient every vehicle leans with the rail under it.
- On the win screen, **A** goes to the next track and **B** goes back to the workshop with the bridge intact, to trim it for more stars.

## Engines and unlocks

Engine select runs in number order.

| No. | Engine | Engine weight | Pulls | Total load | Speed | Unlock |
|-----|--------|---------------|-------|------------|-------|--------|
| 1 | Thomas | 4 | Annie and Clarabel (1 each) | 6 | 1 | Start |
| 2 | Edward | 4 | Tender (2), coach (1) | 7 | ¾ | First clear — Gordon's Hill |
| 3 | Henry | 5 | Tender (2), two trucks (1 each) — the Flying Kipper | 9 | 1 | First clear — Culdee Fell |
| 4 | Gordon | 8 | Tender (2), two express coaches (2 each) | 14 | 1½ | Every track cleared |
| 5 | James | 5 | Tender (2), coach (1) | 8 | 1¼ | First clear — Tidmouth |
| 6 | Percy | 3 | One truck (1) | 4 | 1 | First clear — Knapford |

- Every vehicle is its own load on the bridge, 26 px behind the one in front.
- **Troublesome Trucks** bump: each truck's load swings ±25 % every 8 frames,
  out of step with the next truck. Freight shakes a bridge that coaches would not.
- Engines **slow uphill and speed up downhill**, including on the sag of your
  own deck, so a soft bridge keeps the load on it for longer.
- Each level is **timetabled** for one engine. Any unlocked engine may attempt
  it, but a train lighter than the timetabled one clears the track for **1★** only.
- Gordon is the post-game problem: he breaks every par design except Gordon's
  Hill, where Edward banking gets even him to the top.

Code: [`include/engine.h`](../include/engine.h), [`include/progress.h`](../include/progress.h).

## Levels

Each level offers different solid ground, so each asks for a different structure.

| # | Track | Engine | Tools | Ground you get | What it teaches | Par |
|---|-------|--------|-------|----------------|-----------------|-----|
| 1 | Knapford | Thomas | Timber | Rock on the riverbed | Prop the deck | 4 |
| 2 | Gordon's Hill | Thomas, banked by Edward | Timber + cable | Two crags above a climbing line | Hang the deck; a banker is extra load at the back | 10 |
| 3 | Culdee Fell | Edward | All | Nothing | Warren truss; rods where timber fails | 20 |
| 4 | Tidmouth | Percy | Timber | Nothing | Queen-post truss; why the diagonal matters | 13 |
| 5 | Wellsworth | Henry | All | Ledges low on each wall | Strut from below, under bumping trucks | 9 |
| 6 | Crovan's Gate | Edward | All | One off-centre rock | Trestle plus a free span | 20 |
| 7 | Callan | James | All | A castle tower over each bank | Cable stays and a king post | 15 |
| 8 | Vicarstown | Henry | All | One pier in the channel | The bridge to the mainland: everything, at length | 25 |

Vicarstown is last on purpose: it is the way off the island.

Par is the cost of the reference design in
[`include/level_solutions.h`](../include/level_solutions.h). Host tests prove
each one holds, and that removing any single link from it loses the train.

Stars: cost ≤ par → 3★; ≤ par + par/4 + 1 → 2★; otherwise 1★.

## Lore lines

The odd lines on the result screens are not random. Each answers what you did.

| Line | When |
|------|------|
| **THIN** + banner REALLY USEFUL! | Won at par on the timetabled engine — a bridge with nothing spare |
| banner PEEP PEEP | Won after three or more failures in a row |
| **CHUFF!** | Won first try, over par |
| **LOOP** (win) | Won after 5 or more failures in a row |
| **CONFUS** | Won with a lighter engine than the timetable's |
| **PEEP!** | Any other win |
| **WAIT!** | The bridge collapsed under its own weight before the train left |
| **OOPS!** | Nothing snapped — the deck sagged into the water |
| **AGAIN** | Third failure in a row |
| **LOOP** + banner LISTEN | Fifth failure in a row |
| **ASHES!** + banner CINDERS AND ASHES! | A link snapped under the train |
| banner BUST MY BUFFERS! | Nothing snapped — the train sagged into the water |
| banner BOTHER! FELL DOWN | The bridge came down before the train left |
| banner DELAY! CONFUSION | Third failure in a row: Hatt has stopped being patient |
| **LISTEN** (build HUD) | Sent the train with nothing built at all, instead of NO GO |

The failure-streak lines are the rewind from the [lore](lore-leak.md): every
failed test puts the bridge back exactly as it was, and the game notices.

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
