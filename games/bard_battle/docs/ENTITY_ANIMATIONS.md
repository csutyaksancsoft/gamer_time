# Entity animation assets

Put each animation in its matching folder under `assets/tiled_projects/entities/`. The loader searches these folders recursively. Each `.tsx` tileset must describe one horizontal row of equally sized frames and include one animation. Transparent PNG pixels remain transparent in game.

```text
entities/
├── player_1/
│   ├── death/  ├── idle/  ├── running/  ├── attack/
│   ├── melee/  ├── shield_break/  └── shield/
├── player_2/       # same seven folders
├── player_3/       # same seven folders
├── player_4/       # same seven folders
├── projectile/     # optional non-player projectile strip
└── melee/          # optional non-player melee effect strip
```

Every player TSX needs `bard.player` set to `1` through `4` and `bard.animation` set to `death`, `idle`, `running`, `attack`, `melee`, `shield_break`, or `shield`. Optional positive float properties `bard.world_width` and `bard.world_height` set the rendered world-space size. Non-player projectile and melee-effect sheets continue to use `bard.role=projectile` or `bard.role=melee`.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.10" tiledversion="1.10.2" name="fireball"
         tilewidth="24" tileheight="12" tilecount="3" columns="3">
  <properties>
    <property name="bard.role" value="projectile"/>
    <property name="bard.world_width" type="float" value="18"/>
    <property name="bard.world_height" type="float" value="6"/>
  </properties>
  <image source="fireball.png" width="72" height="12"/>
  <tile id="0"><animation>
    <frame tileid="0" duration="70"/>
    <frame tileid="1" duration="110"/>
    <frame tileid="2" duration="90"/>
  </animation></tile>
</tileset>
```

Player and horizontal movement art faces right. Projectile and melee art must also face right; the client rotates those sheets to the server-authoritative attack angle. Each state owns its complete animation strip; empty/transparent frames are allowed and retain their duration.

## Populating the shield animation

1. Export a transparent PNG containing one horizontal row of equally sized frames.
2. Put it in the desired `entities/player_N/shield/` folder and update its `shield.tsx` so `tilewidth` and `tileheight` equal one frame, `tilecount` and `columns` equal the frame count, and the image width/height match the PNG.
3. Set `bard.player=N` and keep `bard.animation=shield`. Adjust `bard.world_width` and `bard.world_height` only to change its in-game size.
4. Add one `<frame>` entry per tile in playback order. Durations are milliseconds and the active shield animation loops for as long as the server reports the shield active.

The supplied player 1 `block-Sheet.png` is configured as five 32×32 frames at 100 ms each and displays at 40×40 world units.

Each state is independent. Idle and running no longer share a sheet. The supplied single-frame `player_1/idle/idle.png` is configured as player 1's idle pose and renders at 32×32 world units. Empty states fall back to that player's idle; if that is also empty, they fall back to player 1's idle. Death, attack, melee, and shield-break strips play once when their matching network event arrives. Running and shield loop while active.

Malformed files, duplicate roles, and missing roles produce warnings. Players fall back to `player_1` where possible; projectile and melee objects retain their existing geometric placeholders.
