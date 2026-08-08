#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"
exec "$repo_root/build-server/gamer_time_server" \
    --bind "${1:-0.0.0.0}" \
    --map assets/tiled_projects/maps/brawlers_ballad.tmx \
    --songs-dir assets/audio/songs
