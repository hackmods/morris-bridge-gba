# Web playtest

Browser-based verification using **mGBA-WASM** (`@thenick775/mgba-wasm`) — the same core family as [gbajs3](https://gba.nicholas-vancise.dev).

## Why not open `index.html` directly?

mGBA-WASM uses **threads** and `SharedArrayBuffer`. The page must be served with:

```http
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

- **Vite dev server** sets these ([`web/vite.config.ts`](../web/vite.config.ts))
- **Production nginx** sets these ([`deploy/nginx.conf`](../deploy/nginx.conf))
- **`file://` URLs will fail** — always use `npm run dev` or Docker

## Local development

```bash
./scripts/sync_rom_to_web.sh
cd web
npm install
npm run dev
```

Default URL: **http://localhost:4317**

## Features

| Feature | Implementation |
|---------|----------------|
| Auto-load ROM | Fetches `/rom/leo_bridge.gba` on boot |
| Keyboard | [`web/src/keyboard.ts`](../web/src/keyboard.ts) via `bindKey` |
| Command queue | [`web/src/input-queue.ts`](../web/src/input-queue.ts) |
| Live feed | Timestamped log of queued inputs (TPP-style, real-time) |
| ROM upload | File picker + drag-and-drop on canvas |
| Soft reset | Reloads current ROM path |
| Hold duration | Slider 50–200 ms per queued button |

## Command queue

Type space-separated tokens and press **Enqueue**:

```text
right right a down a start
```

Tokens: `up`, `down`, `left`, `right`, `a`, `b`, `start`, `select`, `r`

Unlike original Twitch Plays Pokémon, commands drain **immediately** (serial queue, ~100 ms hold per button).

## Focus handling

When the command `<input>` is focused, `toggleInput(false)` disables mGBA keyboard capture so typed characters appear in the field.

## Production build

```bash
cd web && npm run build
```

Output in `web/dist/`. The root **Dockerfile** runs this build and copies `dist/` into nginx.

## Dependencies

| Package | Role |
|---------|------|
| `vite` | Dev server + bundler |
| `@thenick775/mgba-wasm` | Emulator WASM core |
| `typescript` | Type checking |

WASM assets are emitted as separate chunks (~1.8 MB wasm, gzip ~590 KB).
