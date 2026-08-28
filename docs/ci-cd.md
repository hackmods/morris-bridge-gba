# CI/CD pipeline

Workflow: [`.github/workflows/release.yml`](../.github/workflows/release.yml)

## Triggers

| Event | Behavior |
|-------|----------|
| Push to `main` | Full pipeline + deploy to CapRover |
| Tag `v*` | Full pipeline + deploy with version image tag |
| Pull request | Build ROM + host tests + Docker build (no push/deploy) |
| `workflow_dispatch` | Manual run |

## Jobs

```text
┌─────────────┐     ┌─────────────┐
│  build-rom  │     │ host-tests  │
└──────┬──────┘     └──────┬──────┘
       │                   │
       └─────────┬─────────┘
                 ▼
         ┌──────────────┐
         │ build-image  │  → push to ghcr.io
         └──────┬───────┘
                ▼
         ┌──────────────┐
         │deploy-caprover │  (main + tags only)
         └──────────────┘
```

### `build-rom`

- Checkout repo
- Clone Butano into `vendor/butano`
- Docker: `sunbr0/gba-butano-base:4dd4b16` runs `make`
- Upload `rom/leo_bridge.gba` artifact (14-day retention)

### `host-tests`

- `make -C tests`

### `build-image`

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
docker run --rm -v "$PWD:/project" -w /project sunbr0/gba-butano-base:4dd4b16 \
  bash -lc 'make -j$(nproc) LIBBUTANO=vendor/butano/butano PYTHON=python3'
```

Reproduce CI Docker build:

```bash
./scripts/sync_rom_to_web.sh && docker build .
```
