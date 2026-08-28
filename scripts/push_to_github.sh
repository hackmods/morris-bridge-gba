#!/usr/bin/env bash
# Push the full morris-bridge-gba project from the Cursor cloud backup to GitHub.
#
# Your GitHub repo (hackmods/morris-bridge-gba) may only contain the initial
# README. The complete project (ROM, web, docs, CI) lives on the cloud backup:
#   https://origin.cursor.com/git/hackmods/tmp-599f02a5a560ad00.git
#
# Run this on YOUR machine (not the cloud agent) where you are logged into GitHub.
set -euo pipefail

GITHUB_REPO="${GITHUB_REPO:-hackmods/morris-bridge-gba}"
CLOUD_BACKUP="${CLOUD_BACKUP:-https://origin.cursor.com/git/hackmods/tmp-599f02a5a560ad00.git}"
WORKDIR="${WORKDIR:-morris-bridge-gba-sync}"

echo "Cloning cloud backup (full project)..."
rm -rf "${WORKDIR}"
git clone "${CLOUD_BACKUP}" "${WORKDIR}"
cd "${WORKDIR}"

echo "Adding GitHub remote: ${GITHUB_REPO}"
git remote add github "https://github.com/${GITHUB_REPO}.git" 2>/dev/null || \
  git remote set-url github "https://github.com/${GITHUB_REPO}.git"

echo "Pushing main to GitHub..."
git push github main --force-with-lease

echo ""
echo "Done. Open: https://github.com/${GITHUB_REPO}"
echo "You should see: rom/, web/, docs/, Dockerfile, .github/workflows/release.yml"
