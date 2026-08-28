#!/usr/bin/env bash
# Copy the canonical ROM into the web playtest public folder.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${ROOT}/rom/morris_bridge.gba"
DEST_DIR="${ROOT}/web/public/rom"
DEST="${DEST_DIR}/morris_bridge.gba"

if [[ ! -f "${SRC}" ]]; then
  echo "Missing ${SRC}. Build it first: ./scripts/build_rom.sh" >&2
  exit 1
fi

mkdir -p "${DEST_DIR}"
cp -f "${SRC}" "${DEST}"
echo "Synced ${DEST} ($(wc -c < "${DEST}") bytes)"
