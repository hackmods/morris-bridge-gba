#!/usr/bin/env bash
# Build thomas_bridge.gba using Docker (no local devkitARM required).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE="${GBA_BUILD_IMAGE:-sunbr0/gba-butano-base:4dd4b16}"

cd "$ROOT"

if [[ ! -f vendor/butano/butano/butano.mak ]]; then
  echo "Cloning Butano into vendor/butano ..."
  git clone --depth 1 https://github.com/GValiente/butano.git vendor/butano
fi

echo "Regenerating Thomas's Bridge sprites ..."
python3 tools/gen_graphics.py

echo "Building ROM with ${IMAGE} ..."
sudo docker run --rm -v "$ROOT:/project" -w /project "$IMAGE" \
  bash -lc 'make clean LIBBUTANO=vendor/butano/butano && make -j"$(nproc)" LIBBUTANO=vendor/butano/butano PYTHON=python3'

cp -f thomas_bridge.gba rom/thomas_bridge.gba
"${ROOT}/scripts/sync_rom_to_web.sh"
echo "ROM ready: rom/thomas_bridge.gba ($(wc -c < rom/thomas_bridge.gba) bytes)"
