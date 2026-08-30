# Web playtest

Browser-based verification using **mGBA-WASM** (`@thenick775/mgba-wasm`) — the same core family as [gbajs3](https://gba.nicholas-vancise.dev). This is **not** the game. The game is the Game Boy Advance ROM.

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

Default URL: **http://127.0.0.1:4317** (Vite binds `127.0.0.1` explicitly so Playwright and IPv4-only health checks work).

## Features

| Feature | Implementation |
|---------|----------------|
| Auto-load ROM | Fetches `/rom/thomas_bridge.gba` on boot |
| Keyboard | [`web/src/keyboard.ts`](../web/src/keyboard.ts) via `bindKey` |
| Command queue | [`web/src/input-queue.ts`](../web/src/input-queue.ts) |
| Live feed | Timestamped log of queued inputs (TPP-style, real-time) |
| ROM upload | File picker + drag-and-drop on canvas |
| Soft reset | Reloads current ROM path |
| Hold duration | Slider 50–200 ms per queued button |
| Theater mode | Hides sidebar; canvas fills viewport |
| Dev test hooks | `window.__TEST__` in non-production builds only |

## Command queue

Type space-separated tokens and press **Enqueue**:

```text
right right a down a start
```

Tokens: `up`, `down`, `left`, `right`, `a`, `b`, `start`, `select`, `r`

Unlike original Twitch Plays Pokémon, commands drain **immediately** (serial queue, ~100 ms hold per button).

## Focus handling

When the command `<input>` is focused, `toggleInput(false)` disables mGBA keyboard capture so typed characters appear in the field.

## Dev test hooks (`window.__TEST__`)

Non-production builds install hooks from [`web/src/test-hooks.ts`](../web/src/test-hooks.ts):

| API | Purpose |
|-----|---------|
| `isReady()` | Emulator finished boot |
| `tapButton(name)` | Click UI button by label (e.g. `'Start'`) |
| `enqueue(tokens)` | Push command-queue tokens |
| `waitForCanvasChange(ms)` | Poll screenshot hash for animation |

Production builds omit these hooks. Playwright E2E relies on them for reliable headless gameplay smoke tests.

## E2E tests (Playwright)

```bash
./scripts/sync_rom_to_web.sh   # required — stages ROM under web/public/rom/
cd web
npm ci
npx playwright install chromium
npm run test:e2e
```

Playwright starts Vite on port **4317** via [`web/playwright.config.ts`](../web/playwright.config.ts). Specs live in `web/e2e/`.

See [Testing — Web playtest E2E](testing.md#web-playtest-e2e-playwright) and [Session knowledge — E2E pitfalls](knowledge.md#web--e2e).

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
| `@playwright/test` | E2E browser tests |
| `typescript` | Type checking |
| `vitest` | Unit tests (queue parser, layout) |

WASM assets are emitted as separate chunks (~1.8 MB wasm, gzip ~590 KB).
