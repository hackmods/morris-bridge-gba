#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${ROOT}/rom/thomas_bridge.gba"
DEST_DIR="${ROOT}/web/public/rom"
DEST="${DEST_DIR}/thomas_bridge.gba"

if [[ ! -f "${SRC}" ]]; then
  echo "Missing ${SRC}. Build it first: ./scripts/build_rom.sh" >&2
  exit 1
fi

mkdir -p "${DEST_DIR}"
cp -f "${SRC}" "${DEST}"
echo "Synced ${DEST} ($(wc -c < "${DEST}") bytes)"
