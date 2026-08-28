# Deployment

Ship the **browser playtest** so friends can play without emulators or flash carts.

## Docker image

Multi-stage [`Dockerfile`](../Dockerfile):

1. **web-build** — `npm ci && npm run build` (requires ROM in `web/public/rom/`)
2. **runtime** — `nginx:1.27-alpine` serves static files on port **80**

### Build locally

```bash
./scripts/sync_rom_to_web.sh
docker build -t morris-bridge-gba:local .
docker run --rm -p 8080:80 morris-bridge-gba:local
```

Health check: `GET /health` → `ok`

### CapRover

[`captain-definition`](../captain-definition) points CapRover at the root Dockerfile.

**Container port:** 80  
**HTTPS:** enable in CapRover app settings

Full step-by-step: [`deploy/caprover.md`](../deploy/caprover.md)

## nginx headers (critical)

[`deploy/nginx.conf`](../deploy/nginx.conf) adds COOP/COEP on **all** responses including `.wasm` and `.gba`. Do not place a CDN or proxy in front that strips these headers — mGBA will show a blank canvas.

## Image registry (CI)

GitHub Actions pushes to:

```text
ghcr.io/<owner>/<repo>:latest
ghcr.io/<owner>/<repo>:<sha>
ghcr.io/<owner>/<repo>:v1.0.0   # on version tags
```

CapRover must have **GHCR credentials** configured to pull private packages.

## Secrets required for auto-deploy

| Secret | Purpose |
|--------|---------|
| `CAPROVER_SERVER` | e.g. `captain.yourdomain.com` |
| `CAPROVER_APP_NAME` | CapRover app name |
| `CAPROVER_APP_TOKEN` | Per-app deploy token |

Until secrets exist, CI still builds and pushes the image; only the deploy job fails.

## Optional: GitHub `production` environment

The deploy job uses `environment: production` for manual approval gates before friends receive a new build.

## What friends need

A URL like `https://bridge.yourdomain.com` — any modern desktop browser (Chrome, Firefox, Edge). Mobile browsers may have weaker SharedArrayBuffer support; desktop is recommended.
