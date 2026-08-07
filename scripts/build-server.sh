#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
enet_dir="$repo_root/external/enet"

if [[ ! -f "$enet_dir/CMakeLists.txt" ]]; then
    command -v curl >/dev/null || { echo "curl is required" >&2; exit 1; }
    temporary_dir="$(mktemp -d)"
    trap 'rm -rf "$temporary_dir"' EXIT
    curl -fL "https://github.com/lsalzman/enet/archive/refs/tags/v1.3.18.zip" -o "$temporary_dir/enet.zip"
    (cd "$temporary_dir" && cmake -E tar xf enet.zip)
    mkdir -p "$enet_dir"
    cp -R "$temporary_dir"/enet-1.3.18/. "$enet_dir"
fi

cmake -S "$repo_root" -B "$repo_root/build-server" \
    -DGT_BUILD_CLIENT=OFF -DGT_BUILD_SERVER=ON -DCMAKE_BUILD_TYPE=Release
cmake --build "$repo_root/build-server" --parallel

echo "Server: $repo_root/build-server/gamer_time_server"
echo "Bots:   $repo_root/build-server/gamer_time_bot"
