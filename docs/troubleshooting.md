# Troubleshooting

## GBA build

### `butano.mak: No such file`

`LIBBUTANO` must point at the **inner** `butano/` folder (the one containing `butano.mak`), not the repo root.

```bash
make LIBBUTANO=vendor/butano/butano
# or
make LIBBUTANO=../butano
```

### Docker build fails on `make clean`

Always pass `LIBBUTANO` on **both** `make clean` and `make`:

```bash
make clean LIBBUTANO=vendor/butano/butano && make -j$(nproc) LIBBUTANO=vendor/butano/butano
```

### `DEVKITARM` not found

Install [devkitPro](https://devkitpro.org/) or use `./scripts/build_rom.sh` instead.

## Web playtest

### Blank canvas / WASM error

- Use `npm run dev` — not `file://`
- Check response headers include COOP + COEP
- Use Chrome, Firefox, or Edge desktop

### `ROM not found at /rom/leo_bridge.gba`

```bash
./scripts/sync_rom_to_web.sh
```

### Cannot type in command queue

Fixed in commit `fix(web): allow typing in command queue input` — pull latest `main`. Emulator keyboard is disabled while the command input is focused.

## Docker / CapRover

### Blank screen in production

nginx must send COOP/COEP. Do not strip headers with a CDN. See [`deploy/nginx.conf`](../deploy/nginx.conf).

### `Missing public/rom/leo_bridge.gba`

Run `./scripts/sync_rom_to_web.sh` before `docker build`.

### CapRover deploy fails — missing secrets

Add `CAPROVER_SERVER`, `CAPROVER_APP_NAME`, `CAPROVER_APP_TOKEN`. Build/push still succeeds without them.

### GHCR pull denied

Add registry credentials in CapRover Cluster settings. PAT needs `read:packages`.

## Git / Origin

### `403` pushing to morris-bridge-gba

Cloud agent tokens may lack create/push scope on new repos. Push from your local machine or re-link the Cursor agent to the new repository.

### Remote confusion

See [Repository](repository.md) for `origin` vs `tmp-session`.

## Gameplay

### "NO GO" when starting train

No path from start node (index 0) to end node (index 1). Connect struts into a continuous bridge.

### Train falls immediately

Span too long — max tension rating drops with length. Add intermediate nodes or shorter segments.

### Struts won't connect

- Nodes must be distinct
- Duplicate struts rejected
- Max 32 struts

## CI

### Release workflow deploy job red, others green

Expected until CapRover secrets are configured.

### PR Docker build only (no push)

By design — `push: false` on pull requests.
