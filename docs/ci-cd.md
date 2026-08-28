# CI/CD pipeline

Workflow: [`.github/workflows/release.yml`](../.github/workflows/release.yml)

## Triggers

| Event | Behavior |
|-------|----------|
| Push to `main` | Full pipeline + deploy to CapRover |
| Tag `v*` | Full pipeline + deploy with version image tag |
| Pull request | Build ROM + all tests + Docker build (no push/deploy) |
| `workflow_dispatch` | Manual run |

## Jobs

```text
┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│  build-rom  │  │ host-tests  │  │ web-tests   │
└──────┬──────┘  └──────┬──────┘  └──────┬──────┘
       │                │                │
       └────────────────┼────────────────┘
                        │
              ┌─────────▼─────────┐
              │     web-e2e       │  (needs ROM artifact)
              └─────────┬─────────┘
                        ▼
              ┌──────────────┐
              │ build-image  │  → push to ghcr.io
              └──────┬───────┘
                     ▼
              ┌──────────────┐
              │deploy-caprover│  (main + tags only)
              └──────────────┘
```

### `build-rom`

- Checkout repo
- Clone Butano into `vendor/butano`
- Run `python3 tools/gen_graphics.py`
- Docker: `sunbr0/gba-butano-base:4dd4b16` runs `make`
- Copy `thomas_bridge.gba` → `rom/thomas_bridge.gba`
- Upload `rom/thomas_bridge.gba` artifact (14-day retention)

### `host-tests`

- `make -C tests`

### `web-tests`

- Node 22, `npm ci` in `web/`
- `npm test` (Vitest: command queue + layout helpers)

### `web-e2e`

- Depends on `build-rom` artifact
- Stages ROM to `web/public/rom/thomas_bridge.gba`
- Installs Playwright Chromium (`npx playwright install --with-deps chromium`)
- `npm run test:e2e` (12 specs: ROM boot, rendering, command queue, gameplay)
- On failure: uploads `playwright-report/` artifact (7-day retention)

### `build-image`

- Needs: `build-rom`, `host-tests`, `web-tests`, `web-e2e`
- Download ROM artifact → `web/public/rom/`
- `docker/build-push-action` with GHA cache
- Push to GHCR unless PR

### `deploy-caprover`

- `caprover/deploy-from-github@v1.1.2`
- Image: `ghcr.io/<repo>:latest` or tag name on `v*` releases

## Permissions

```yaml
permissions:
  contents: read
  packages: write
```

`GITHUB_TOKEN` is used for GHCR login.

## Concurrency

One release per ref; newer pushes cancel in-progress runs on the same branch.

## Replacing the old workflow

The previous `build-gba.yml` (ROM-only) was merged into this pipeline. There is no separate ROM workflow.

## Local parity

Reproduce CI ROM build:

```bash
git clone --depth 1 https://github.com/GValiente/butano.git vendor/butano
python3 tools/gen_graphics.py
docker run --rm -v "$PWD:/project" -w /project sunbr0/gba-butano-base:4dd4b16 \
  bash -lc 'make -j$(nproc) LIBBUTANO=vendor/butano/butano PYTHON=python3'
cp -f thomas_bridge.gba rom/thomas_bridge.gba
```

Reproduce CI web tests:

```bash
cd web && npm ci && npm test
./scripts/sync_rom_to_web.sh && cd web && npx playwright install chromium && npm run test:e2e
```

Reproduce CI Docker build:

```bash
./scripts/sync_rom_to_web.sh && docker build .
```

See [Session knowledge — Known pitfalls](knowledge.md#known-pitfalls-do-not-re-learn) for E2E troubleshooting.
