# Modern workshop (product)

The playable game is a **high-resolution browser workshop**, not the GBA ROM.

| Want | Go |
|------|----|
| Play | `cd web && npm run dev` → http://127.0.0.1:4317 |
| GBA prototype | `/legacy.html` or `rom/thomas_bridge.gba` |

## Controls

- Drag from a joint to weld a span. Keep dragging: joints plant automatically when the stroke gets long
- `1` / `2` / `3` road, timber, steel
- `E` erase, `U` or Ctrl+Z undo, Enter test

Road is the only driveable material. Timber and steel only hold the deck up.
Hot colour on a placed span (or the weld ghost) means it is near its length or load limit.
