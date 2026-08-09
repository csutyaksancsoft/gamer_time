#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"
exec "$repo_root/build-server/games/bard_battle/bard_battle_server" \
    --bind "${1:-0.0.0.0}" \
    --map games/bard_battle/assets/tiled_projects/maps/brawlers_ballad.tmx \
    --songs-dir games/bard_battle/assets/audio/songs
