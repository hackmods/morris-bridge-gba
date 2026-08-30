# Thomas's Bridge

A **modern bridge builder** for the browser. Drag joints, weld a road, brace it with timber and steel, then send an engine across.

The old Game Boy Advance prototype remains at `/legacy.html` and in `rom/thomas_bridge.gba` — it is no longer the product.

## Play

```bash
cd web
npm install
npm run dev
```

Open **http://127.0.0.1:4317**

- **Drag** from a joint to weld a span
- **Road** is the only surface the train can drive
- **Timber / steel** are structure — they do not carry the train
- **Test crossing** when a road links START to END

## Campaign

Eight crossings: Knapford Creek through The Express. Unlock Percy, Edward, James, and Gordon as you clear tracks. Heavier engines demand tighter triangles.

## Tests

```bash
cd web && npm test           # physics, welding, reference playthroughs
cd web && npm run test:e2e   # workshop UI
make -C tests                # original GBA host tests (optional)
```

## Documentation

GBA-era notes still live in [`docs/`](docs/README.md). The playable game is the web workshop.
