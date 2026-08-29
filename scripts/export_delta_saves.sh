#!/usr/bin/env bash
# Write playtest SRAM files for Delta / mGBA import (thomas_bridge.sav naming).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/rom/delta_saves}"
python3 "$ROOT/tools/gen_playtest_save.py"
mkdir -p "$OUT"
cp "$ROOT/web/public/saves/gordon_hill_walkthrough.sav" "$OUT/thomas_bridge_gordon_hill.sav"
cp "$ROOT/web/public/saves/all_unlocked.sav" "$OUT/thomas_bridge_all_unlocked.sav"
echo "Wrote Delta import saves to $OUT/"
echo "  thomas_bridge_gordon_hill.sav  — H3 shortcut (Knapford cleared)"
echo "  thomas_bridge_all_unlocked.sav — all levels + stars (Vicarstown/H6 spot checks)"
echo "Rename chosen file to thomas_bridge.sav before importing into Delta."
