# Repository guide

## GitHub (hackmods/morris-bridge-gba)

If you created the repo on **GitHub** (private, via mobile or web), it may only contain the initial `README.md`. The **full project** is on the Cursor cloud backup until you sync:

| Remote | URL | Contents |
|--------|-----|----------|
| Cloud backup (full) | `https://origin.cursor.com/git/hackmods/tmp-599f02a5a560ad00.git` | All commits: ROM, web, docs, CI |
| GitHub (yours) | `https://github.com/hackmods/morris-bridge-gba.git` | May be README-only until push |

### One-command sync (run on your Mac/PC)

```bash
./scripts/push_to_github.sh
```

Or manually:

```bash
git clone https://origin.cursor.com/git/hackmods/tmp-599f02a5a560ad00.git morris-bridge-gba-full
cd morris-bridge-gba-full
git remote add github https://github.com/hackmods/morris-bridge-gba.git
git push github main --force-with-lease
```

The cloud agent **cannot** push to GitHub directly (no `gh` auth / 403 on private repo). Run the sync locally once.

## Origin (Cursor)

| Field | Value |
|-------|-------|
| **Name** | `morris-bridge-gba` |
| **Origin URL** | `https://origin.cursor.com/git/hackmods/morris-bridge-gba.git` |
| **Default branch** | `main` |
| **ROM codename** | `leo_bridge` |

## Remotes

This project was bootstrapped in a Cursor **New Project** session. You may see two remotes:

| Remote | Purpose |
|--------|---------|
| `origin` | `morris-bridge-gba` (permanent private repo) |
| `tmp-session` | Temporary session remote (backup during migration) |

### First-time push to morris-bridge-gba

```bash
git remote set-url origin https://origin.cursor.com/git/hackmods/morris-bridge-gba.git
git push -u origin main
```

### Mirror from session remote

If `morris-bridge-gba` is empty:

```bash
git clone https://origin.cursor.com/git/hackmods/tmp-599f02a5a560ad00.git morris-bridge-gba
cd morris-bridge-gba
git remote set-url origin https://origin.cursor.com/git/hackmods/morris-bridge-gba.git
git push -u origin main
```

## Branch policy

- **`main`** — integration branch; triggers full Release workflow
- **Tags `v*`** — semver releases; deploy with version image tag
- **PRs** — build + test only (no GHCR push, no CapRover deploy)

## What is tracked in git

| Tracked | Ignored |
|---------|---------|
| Source, graphics, docs | `build/`, `vendor/` |
| `rom/leo_bridge.gba` (prebuilt) | `*.elf`, root `*.gba` except `rom/` |
| `web/` source + lockfile | `web/node_modules/`, `web/dist/` |
| CI, Docker, CapRover config | `web/public/rom/*.gba` (synced at build) |

## GitHub / GHCR naming

When mirrored to GitHub, the Release workflow publishes:

```text
ghcr.io/<github-owner>/morris-bridge-gba:latest
```

Image name follows `${{ github.repository }}` (lowercased at deploy).

## Related files

- [`REPO.md`](../REPO.md) — short connection cheat sheet
- [`docs/ci-cd.md`](ci-cd.md) — automation details
