# Deploy Thomas's Bridge playtest to CapRover

Friends can play the GBA web emulator at your CapRover URL once this is set up. The
[`release.yml`](../.github/workflows/release.yml) workflow builds the ROM, packages the
static site into a Docker image, pushes to GitHub Container Registry (GHCR), and deploys
to CapRover on every push to `main` or version tag (`v*`).

## 1. Create the CapRover app

1. In CapRover, create a new app (e.g. `thomas-bridge`).
2. Enable **HTTPS** and set your domain (e.g. `bridge.yourdomain.com`).
3. Under **HTTP Settings**, leave the container port at **80**.
4. Under **App Configs → Container HTTP Settings**, no special env vars are required.

## 2. Allow CapRover to pull from GHCR

1. In CapRover → **Cluster** → **Docker Registry**.
2. Add GitHub Container Registry:
   - Registry domain: `ghcr.io`
   - Username: your GitHub username
   - Password: a GitHub PAT with `read:packages`
3. Save.

## 3. GitHub repository secrets

In GitHub → **Settings → Secrets and variables → Actions**, add:

| Secret | Example | Purpose |
|--------|---------|---------|
| `CAPROVER_SERVER` | `captain.yourdomain.com` | CapRover dashboard host (no `https://`) |
| `CAPROVER_APP_NAME` | `thomas-bridge` | App name from step 1 |
| `CAPROVER_APP_TOKEN` | *(from CapRover app settings)* | Deploy token for that app |

Create the CapRover app token: open the app → **Deployment** tab → enable
**"App Token"** and copy the value.

## 4. GitHub environment (optional but recommended)

Create an environment named **`production`** in GitHub → **Settings → Environments**.
The deploy job uses this so you can add approval gates before friends get a new build.

## 5. First deploy

Push to `main` or run the **Release** workflow manually (**Actions → Release → Run workflow**).

The pipeline:

```text
build-rom → host-tests → build-image (GHCR) → deploy-caprover
```

Image tags pushed to GHCR:

- `latest` (main branch)
- `main` (branch ref)
- `v1.2.3` (semver tags)
- short SHA

## 6. Manual deploy (without CI)

If you build locally:

```bash
./scripts/sync_rom_to_web.sh
docker build -t morris-bridge-gba:local .
docker run --rm -p 8080:80 morris-bridge-gba:local
# open http://localhost:8080
```

Or deploy from CapRover using the repo root `captain-definition` + `Dockerfile` after
running `./scripts/sync_rom_to_web.sh`.

## Troubleshooting

### Emulator shows a blank screen

mGBA-WASM requires cross-origin isolation. The nginx config in [`nginx.conf`](nginx.conf)
sets `Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` on all responses.
Do not put a CDN or reverse proxy in front that strips these headers.

### Deploy job fails with missing secrets

Add the three `CAPROVER_*` secrets above. Until they exist, the build and GHCR push
jobs still run; only deploy fails.

### GHCR pull denied on CapRover

Ensure the registry credentials in step 2 are correct and the package is visible to your
GitHub user (public package, or PAT with package read access).
