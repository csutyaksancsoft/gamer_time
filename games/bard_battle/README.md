# Bard Battle

Bard Battle is a networked rhythm-combat arena built with Attempt Engine. Players move with WASD and time shots or shields to the song. An authoritative server manages movement, projectiles, rounds, mode/song voting, teams, scoring, respawns, and observer sound visibility; the client renders the arena, UI, audio, and cosmetic fog of war.

## Layout

```text
assets/      Maps, tiles, audio, fonts, and UI resources
docs/        Asset workflow and historical implementation notes
scripts/     Windows client and Linux server build/launch scripts
src/         Client, server, bot, protocol, rhythm, match, and UI code
tests/       Bard Battle protocol and gameplay tests
tools/       Song-analysis utilities
```

Open `assets/tiled_projects/bard_battle.tiled-project` in Tiled to edit the default map. The detailed authoring guide is in `assets/tiled_projects/maps/README.md`.

## Build

From the repository root, build the Windows client and portable ZIP with:

```powershell
.\games\bard_battle\scripts\build-windows.ps1
```

Build and run the Linux dedicated server with:

```bash
bash games/bard_battle/scripts/build-server.sh
bash games/bard_battle/scripts/run-lan-server.sh
```

Primary outputs are `bard_battle`, `bard_battle_server`, and `bard_battle_bot`. The network port remains UDP `27020`.

Client options:

```text
--server IP --name Player --calibration-ms N
```

Server startup options:

```text
--bind IP --map PATH --songs-dir PATH --help
```

The server console supports `status`, `songs`, `start`, `stop`, `set`, `kick`, `help`, and `quit`.
