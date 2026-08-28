#!/usr/bin/env bash
# Build leo_bridge.gba using Docker (no local devkitARM required).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE="${GBA_BUILD_IMAGE:-sunbr0/gba-butano-base:4dd4b16}"

cd "$ROOT"

if [[ ! -f vendor/butano/butano/butano.mak ]]; then
  echo "Cloning Butano into vendor/butano ..."
  git clone --depth 1 https://github.com/GValiente/butano.git vendor/butano
fi

echo "Building ROM with ${IMAGE} ..."
docker run --rm -v "$ROOT:/project" -w /project "$IMAGE" \
  bash -lc 'make clean LIBBUTANO=vendor/butano/butano && make -j"$(nproc)" LIBBUTANO=vendor/butano/butano PYTHON=python3'

cp -f leo_bridge.gba rom/leo_bridge.gba
"${ROOT}/scripts/sync_rom_to_web.sh"
echo "ROM ready: rom/leo_bridge.gba ($(wc -c < rom/leo_bridge.gba) bytes)"
