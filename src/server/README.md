# Gamer Time Dedicated Server

This directory contains the headless authoritative server and its command-line
configuration code. The server owns player movement, combat, match state,
mode voting, song voting, rhythm judgments, and song scheduling.

The examples in this document assume commands are run from the repository
root. Paths passed to the server are resolved from the process's current
working directory.

## Build and run

Build the Linux server and bot without building SDL or Vulkan client code:

```bash
bash scripts/build-server.sh
```

Run it with the repository's standard LAN configuration:

```bash
bash scripts/run-lan-server.sh
```

The launch script accepts an optional bind address as its first argument:

```bash
bash scripts/run-lan-server.sh 192.168.1.10
```

You can also run the binary directly:

```bash
./build-server/gamer_time_server --songs-dir assets/audio/songs
```

Use `--help` to print the short built-in reference:

```bash
./build-server/gamer_time_server --help
```

## Boot-time options

Normal operation uses only three boot-time values: the socket bind address,
map path, and song catalog directory. Gameplay configuration should be changed
with live `set` commands, so an operator does not need to restart the server.

Gameplay flags are deliberately not accepted at process launch. Launch once,
then use the terminal to configure and run as many rounds as needed.

### `--bind IP`

Selects the network interface on which the server listens. The UDP port is
always `27020`; do not include it in the argument.

- Default: `0.0.0.0`
- `0.0.0.0` listens on every available interface.
- Use `127.0.0.1` when testing locally only.
- Use a LAN address when the server should listen on one specific adapter.

```bash
./build-server/gamer_time_server --bind 0.0.0.0
```

### `--map PATH`

Selects the TMX map loaded at startup.

- Default: `assets/maps/grass_tileset_map.tmx`
- A missing or malformed map prevents startup.

```bash
./build-server/gamer_time_server --map assets/maps/grass_tileset_map.tmx
```

### `--songs-dir PATH`

Selects the song catalog directory.

- Default: `assets/audio/songs`
- Every regular `*.cfg` file directly inside the directory is loaded.
- The catalog must contain at least one valid song.
- Every song ID must be unique.
- Every referenced WAV must exist.
- WAV paths in configs are relative to the config containing them.

```bash
./build-server/gamer_time_server --songs-dir assets/audio/songs
```

## Live gameplay settings

Enter these commands in the running server terminal. Changes become the
defaults for subsequent `start` commands. `friendly-fire` affects combat
immediately; the other settings are consumed when the next round starts.

### `set mode-vote on|off`

Controls whether players vote between Teams and Free-for-All at the start of
each round.

- Default: `on`
- With mode voting off, `set mode` supplies the selected mode.
- Players may change their vote until the full 15-second timer expires.
- A tied mode vote resolves to Free-for-All.

```text
set mode-vote off
```

### `set mode ffa|teams`

Sets the mode used when mode voting is disabled.

- Default: `ffa`
- Accepted values are exactly `ffa` and `teams`.

```text
set mode teams
```

### `set song-vote on|off`

Controls whether players vote on a shuffled song ballot.

- Default: `on`
- A one-song catalog automatically skips song voting.
- A live `set force-song ID` also skips song voting.
- When disabled without a forced song, the server selects from its shuffle
  rotation.
- Song votes remain changeable until the 15-second timer expires.
- Tied leaders are resolved randomly, including the all-zero-vote case.

```text
set song-vote off
```

### `set song-count N`

Sets the default number of automatically shuffled song candidates.

- Default: `3`
- Valid range: `1` through `8`
- Candidates in one ballot are distinct.
- If the catalog has fewer songs than requested, the ballot contains all
  available songs.
- The shuffle bag avoids repeating candidates until all catalog songs have
  appeared.

```text
set song-count 5
```

### `set force-song ID|off`

Persistently selects one song for every round and skips song voting. Mode
voting remains independent and may still run.

The ID must exist in the loaded catalog.

```text
set force-song main
set force-song off
```

### `set team-count 2|3|4`

Sets the number of selectable teams.

- Default: `2`
- Valid range: `2` through `4`

```text
set team-count 4
```

### `set friendly-fire on|off`

Controls whether projectiles can damage a player on the shooter's own team.
This setting matters in Teams mode.

- Default: `off`

```text
set friendly-fire on
```

### `--help`

Prints the built-in boot and live-command summary and exits successfully.

```bash
./build-server/gamer_time_server --help
```

## Song catalog format

Keep each song config and its WAV together under the songs directory:

```text
assets/audio/songs/
├── main.cfg
├── MEMECAR-001.wav
├── second_song.cfg
└── second_song.wav
```

Example config:

```ini
id=main
file=./MEMECAR-001.wav
bpm=120
first_beat_ms=500
subdivision=1
duration_ms=145000
```

Fields:

- `id`: unique network-visible song identifier.
- `file`: WAV path relative to this config file.
- `bpm`: positive beats-per-minute value.
- `first_beat_ms`: first beat offset from the beginning of the WAV.
- `subdivision`: positive number of rhythm notes generated per beat.
- `duration_ms`: positive track duration in milliseconds.

The server rejects an empty catalog, duplicate IDs, malformed config lines,
missing required values, non-positive rhythm values, and missing WAV files.

Clients do not download music from the server. Every client distribution must
contain the same song IDs and matching audio assets. If a client receives a
scheduled ID it does not have, it reports an audio error instead of playing a
different track.

## Live console commands

Type these commands into the terminal running `gamer_time_server`.

### `help`

Prints the short boot, `set`, and live `start` reference.

```text
help
```

### `status`

Prints a one-line operational summary containing:

- catalog size;
- connected player count;
- room state;
- current live mode/song vote settings;
- default song candidate count;
- currently active vote sections;
- active candidate count;
- selected mode and song; and
- remaining voting time.

Room state is currently printed numerically:

| Value | State |
|---:|---|
| `0` | Lobby |
| `1` | Voting |
| `2` | Three-second countdown |
| `3` | Playing the scheduled song |
| `4` | Free movement / idle between rounds |

```text
status
```

### `songs`

Lists every loaded song ID with BPM, duration, and resolved WAV path. Use this
before entering explicit IDs in `start --songs` or `start --force-song`.

```text
songs
```

### `start`

Starts a round using the current live settings.

The command is refused unless:

- at least one player is connected;
- every connected client has completed its ready handshake; and
- the room is not already voting, counting down, or playing.

Active players are snapshotted when voting begins. Disconnecting players are
removed from eligibility. Players joining during a countdown or active round
wait for the next round.

```text
start
```

### `start --mode-vote`

Enables mode voting for this round, regardless of the current live setting.

```text
start --mode-vote
```

### `start --no-mode-vote --mode ffa|teams`

Disables mode voting for this round and selects its mode directly. Supplying
`--mode` while mode voting remains enabled is rejected as ambiguous.

```text
start --no-mode-vote --mode teams
```

### `start --song-vote`

Enables automatic song voting for this round. The default candidate count is
inherited from the live setting unless `--song-count` or `--songs` is supplied.

```text
start --song-vote
```

### `start --song-vote --song-count N`

Enables song voting and draws `N` automatic candidates from the shuffle bag.
`N` must be from 1 through 8.

```text
start --song-vote --song-count 5
```

### `start --song-vote --songs ID,ID,...`

Uses an explicit candidate ballot for this round. IDs are comma-separated
without spaces, and console order is preserved.

Rules:

- Song voting must be enabled.
- The list must contain 1 through 8 IDs.
- Every ID must exist in the catalog.
- Duplicate IDs are rejected.

```text
start --song-vote --songs main,second_song,third_song
```

### `start --no-song-vote`

Disables song voting for this round. Without a forced song, the server chooses
the next song from its shuffle rotation.

```text
start --no-song-vote
```

### `start --no-song-vote --force-song ID`

Disables song voting and selects one known song for this round only. Mode
voting is unaffected unless separately overridden.

```text
start --no-song-vote --force-song main
```

`--force-song` cannot be combined with `--songs` or `--song-count`.

### Combined round overrides

Mode and song voting are independent. Both sections share one 15-second phase
when both are enabled. If only one is enabled, only that section is shown. If
neither is enabled, the server immediately resets round statistics and starts
the existing three-second countdown for the selected song.

```text
start --mode-vote --song-vote --songs main,second_song,third_song
```

One-round overrides are parsed for that invocation and do not modify live
settings. A refused or invalid start does not change the next round's settings.

### `stop`

Moves the room to the free-movement state. It does not shut down the process.

```text
stop
```

### `kick ID`

Disconnects the player with the specified numeric player ID. Player IDs are
shown in join messages printed by the server.

```text
kick 3
```

### `quit`

Stops the server loop, closes the ENet host, and exits the process.

```text
quit
```

## Voting and round timing

The normal round sequence is:

1. An operator enters `start` with optional one-round overrides.
2. The server snapshots eligible active players.
3. Enabled vote sections remain open for the full 15 seconds.
4. The server resolves enabled votes at timeout.
5. Round kills, deaths, and rhythm statistics reset once.
6. The selected song is preloaded and scheduled three seconds ahead.
7. The countdown ends and authoritative rhythm gameplay begins.
8. When the song duration expires, the room enters free movement.

Players may change either vote until timeout. Song ties are randomized among
all tied leaders. Mode ties use Free-for-All. Forced songs skip only song
voting, so a mode vote can still occur normally.

## Common errors

### `Start refused: all connected clients must be ready and the room idle`

Wait for clients to finish connecting, or wait until the current voting,
countdown, or playing phase ends. Use `status` to inspect the room.

### `Unknown song id`

Run `songs` and use the exact ID shown. IDs are taken from the `id=` field, not
from the WAV filename.

### `--mode requires mode voting to be disabled`

Use `start --no-mode-vote --mode teams` or omit `--mode` and let players vote.

### `--songs requires song voting`

Add `--song-vote`, or remove the explicit candidate list.

### `--force-song conflicts with --songs and --song-count`

Choose either one forced song or a song-vote ballot configuration.

### Server and client disagree about a song

Distribute matching `assets/audio/songs` directories with both packages. Song
audio is never transferred over the network.

## Quick reference: copy and paste

The commands below are intentionally collected at the bottom for fast access.

### Build and basic launch

```bash
bash scripts/build-server.sh
bash scripts/run-lan-server.sh
bash scripts/run-lan-server.sh 0.0.0.0
./build-server/gamer_time_server --help
./build-server/gamer_time_server --songs-dir assets/audio/songs
```

### Local-only and LAN bindings

```bash
./build-server/gamer_time_server --bind 127.0.0.1 --songs-dir assets/audio/songs
./build-server/gamer_time_server --bind 0.0.0.0 --songs-dir assets/audio/songs
./build-server/gamer_time_server --bind 192.168.1.10 --songs-dir assets/audio/songs
```

### Configure the running server

```text
set mode-vote on
set mode-vote off
set mode ffa
set mode teams
set song-vote on
set song-vote off
set song-count 1
set song-count 3
set song-count 5
set song-count 8
set force-song main
set force-song off
set team-count 2
set team-count 4
set friendly-fire on
set friendly-fire off
status
```

### Live inspection and administration

```text
help
status
songs
kick 3
stop
quit
```

### Start with current live settings

```text
start
```

### Mode-only round controls

```text
start --mode-vote
start --no-mode-vote --mode ffa
start --no-mode-vote --mode teams
```

### Song-only round controls

```text
start --song-vote
start --song-vote --song-count 1
start --song-vote --song-count 3
start --song-vote --song-count 5
start --song-vote --song-count 8
start --song-vote --songs main,second_song,third_song
start --no-song-vote
start --no-song-vote --force-song main
```

### Combined round controls

```text
start --mode-vote --song-vote
start --mode-vote --song-vote --song-count 5
start --mode-vote --song-vote --songs main,second_song,third_song
start --no-mode-vote --mode teams --song-vote --song-count 3
start --mode-vote --no-song-vote --force-song main
start --no-mode-vote --mode ffa --no-song-vote --force-song main
start --no-mode-vote --mode teams --no-song-vote
```

Replace example IDs such as `second_song` and `third_song` with IDs printed by
the live `songs` command.
