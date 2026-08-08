# Bard Battle dedicated server

Build and launch from the repository root:

```bash
bash games/bard_battle/scripts/build-server.sh
bash games/bard_battle/scripts/run-lan-server.sh
```

The default map is `games/bard_battle/assets/tiled_projects/maps/brawlers_ballad.tmx`; songs are read from `games/bard_battle/assets/audio/songs`. Run `./build-server/games/bard_battle/bard_battle_server --help` for startup options and use the interactive `help` command for live round controls.

## Live server commands

Enter these in the running server console:

```text
set mode-vote on|off
set song-vote on|off
set mode ffa|teams
set song-count 1-8
set force-song ID|off
set team-count 2-4
set friendly-fire on|off
set shield-freeze on|off
set shield-melee stun|kill
```

`shield-freeze` defaults to `on`. While it is on, shielding stops player movement until the one-beat shield expires or is broken. `shield-melee` defaults to `stun`: a melee hit breaks the shield and locks that player out of movement, shooting, shielding, and melee for two beats. Set it to `kill` to make a melee hit eliminate a shielded player immediately.

Round commands are `start` (with the options shown by `help`) and `stop`. Other commands are `status`, `songs`, `kick ID`, `help`, and `quit`. The `status` command includes the current shield settings.
