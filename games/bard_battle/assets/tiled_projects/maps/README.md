# Map authoring guide

This project reads Tiled TMX maps. Use these settings when creating a map:

- **Orientation:** Orthogonal
- **Map size:** Finite (disable Infinite)
- **Tile size:** 32 × 32 pixels for the current art
- **Tile Layer format:** CSV, uncompressed

Keep maps in `assets/tiled_projects/maps/` and tilesheet images with their external tilesets in `assets/tiled_projects/tiles/`. A typical layout is:

```text
assets/tiled_projects/
├── maps/
│   └── my_arena.tmx
└── tiles/
    └── terrain/
        ├── terrain.png
        └── terrain.tsx
```

## Create the map and tilesets

1. In Tiled, choose **File → New → New Map**, select Orthogonal, disable Infinite, set 32 × 32 tiles, and save the TMX under `assets/tiled_projects/maps/`.
2. In Map Properties, set Tile Layer Format to CSV and Compression to None.
3. For each sheet, choose **File → New → New Tileset**, select **Based on Tileset Image**, and set its tile size, margin, and spacing to match the PNG.
4. Save each tileset as an external `.tsx` beside its PNG in a category such as `terrain`, `water`, or `props`, then add it to the map. Use lowercase, normalized filenames (for example, `castle_walls.tsx`) and keep TMX/TSX/image paths relative. Save the TSX after every animation or metadata change.

Image-collection tilesets are unsupported. Every tileset used by one map must use the map's tile size; sheets may have different image dimensions and column counts.

## Tile Layers and animation

Create ordinary Tile Layers for ground, water, walls, and other grid-aligned art. Their order in Tiled is their visual order. Paint static tiles normally.

To animate a tile, open its external TSX, select the representative tile, and add frames in the **Tile Animation Editor** with durations in milliseconds. Frames must belong to that same tileset. Save the TSX, enable **View → Show Tile Animations** to preview, then paint the representative tile onto a Tile Layer.

## Visual tile objects

For freely placed props, create a non-collision Object Layer and set its draw order to **Top Down**. Use **Insert Tile** to place a tileset tile. Tile objects render with players and are sorted by their ground position, so a player can pass visually behind or in front of them. They may be moved, resized, rotated, and flipped/diagonally transformed. A visual tile object never creates collision.

Keep visual Object Layers visible and give them useful names such as `props` or `canopy`. Layer and object opacity/visibility affect rendering only; invisible collision layers remain active in gameplay.

Tile layers render below players by default. To make a tile layer act as foreground, add a custom string property named `render_phase` with the value `above_units`. Foreground behavior is property-driven, so the layer can be named freely. The values `below_units` and an omitted or unrecognized value render below players.

## Collision

Collision is authored with shapes on specially named Object Layers. In Tiled:

1. In the **Layers** panel, choose **New Object Layer**.
2. Give the layer one of the exact, case-sensitive names in the table below.
3. Select **Insert Rectangle** for box-shaped walls or **Insert Polygon** for irregular and angled boundaries.
4. Draw over the solid part of the map. A polygon must contain at least three points.
5. Repeat with another collision layer when different parts of the map need different behavior.

Choose the layer according to what the shape should block:

| Object Layer | Players | Shots | Vision / observer sound |
|---|:---:|:---:|:---:|
| `collision_full` | Blocked | Blocked | Blocked |
| `collision_shots` | Blocked | Blocked | Passes |
| `collision_player` | Blocked | Passes | Passes |

Use `collision_full` for opaque walls, buildings, and cliffs that should also obstruct fog of war and prevent distant observers from hearing events through them. Use `collision_shots` when players and attacks must stop but sight should pass. Use `collision_player` for railings, low obstacles, or arena limits that players cannot cross but attacks and sight can.

Keep collision rectangles unrotated. Rectangle rotation is not applied by the collision system, so draw a polygon when an edge needs to be angled. Point objects, tile objects, zero-sized rectangles, and malformed polygons do not collide. Object names, types, and custom properties do not change collision behavior; the Object Layer name is the only selector. Legacy `collision` or `boundary` layers and `engine.*` properties are ignored.

Collision layers may be hidden in Tiled to make editing easier. Layer and object visibility do not disable their gameplay collision. In the client, press **F3** to display every active collision channel and confirm that the shapes line up with the map art.

## Respawn zones

Every Bard Battle map must contain exactly one Object Layer named `respawn_zone_1` through `respawn_zone_4`. All four exact, case-sensitive layers are mandatory even when the current match uses fewer teams. A typical layer list looks like this:

```text
Object Layers
├── collision_full
├── collision_shots
├── collision_player
├── respawn_zone_1
├── respawn_zone_2
├── respawn_zone_3
└── respawn_zone_4
```

Create each respawn zone in Tiled as follows:

1. Choose **New Object Layer** and enter the exact `respawn_zone_N` name.
2. Use **Insert Rectangle** to draw one or more unrotated spawn areas. Use **Insert Polygon** for an irregular area.
3. Make every shape large enough to contain the complete player body. Players have a radius of 8 world pixels, so a spawn center needs at least 8 pixels of clear space from every shape edge.
4. Keep the usable interior clear of `collision_full`, `collision_shots`, and `collision_player` geometry because all three block the player channel.
5. Repeat until all four required layers exist. The layers may be hidden after authoring.

Each layer must contain at least one usable positive-area shape. Supported shapes are non-rotated rectangles and polygons with at least three valid points. Point and tile objects, rotated or zero-sized rectangles, and malformed polygons are ignored; object names, types, custom properties, and layer visibility do not affect spawning. The server validates the zones when it loads the map and refuses to start if a layer is missing, duplicated, has no supported shape, or has no location where the complete player bounds fit without touching player-channel collision.

FFA spawning combines all four `respawn_zone_N` layers. Team spawning uses only `respawn_zone_N` for team N, so a two-team match uses zones 1 and 2. Multiple shapes in a selected layer form one area and are chosen proportionally by area. Spawn layers may be hidden in Tiled and remain active in gameplay.

When several players are alive, the server tries to choose a valid point at least 64 world pixels from them. If every candidate is crowded, it uses the valid point with the greatest available separation rather than spawning inside collision.

## Collision and respawn troubleshooting

If a map does not behave as expected, check these common causes:

- **A collision shape has no effect:** confirm its Object Layer is named exactly `collision_full`, `collision_shots`, or `collision_player`. Names are case-sensitive. Make sure the object is a positive-size rectangle or a polygon with at least three points, not a point or tile object.
- **Angled rectangle collision is wrong:** replace the rotated rectangle with a polygon matching the desired outline.
- **The server reports a missing respawn layer:** create all four `respawn_zone_1` through `respawn_zone_4` layers, even for maps intended for FFA or two teams, and check capitalization.
- **The server reports a duplicate respawn layer:** merge the shapes into one layer and remove or rename the duplicate. Multiple shapes are allowed, but multiple layers with the same required name are not.
- **The server reports that a respawn layer contains no usable shape:** remove rotation from rectangles, give them positive width and height, or replace malformed polygons with polygons containing at least three valid points.
- **The server reports no collision-free location:** enlarge the zone or move it away from player-blocking collision. Leave at least 8 pixels between a possible spawn center and every zone edge, and enough clear interior for the full 16 × 16 player bounds.
- **Hidden layers seem active:** this is intentional. Visibility is for editing and rendering only; collision and respawn layers remain active when hidden.

## Unsupported features

- Infinite/chunked maps
- Base64 or compressed Tile Layer data
- Image-collection tilesets
- Mixed tile sizes within a map

## Before launching

- Confirm all TMX → TSX → PNG paths are relative and resolve from their containing file.
- Save every external TSX and verify animations with **Show Tile Animations**.
- Confirm visual layers that should render are visible.
- Confirm Tile Layers use uncompressed CSV data.
- Check collision layer spelling and capitalization exactly.
- Confirm all four respawn layer names exist exactly once.
- Check that each respawn layer has a clear, positive-area rectangle or polygon.
- Press F3 in the client and inspect collision alignment before distributing the map.

The default map is `games/bard_battle/assets/tiled_projects/maps/brawlers_ballad.tmx` from the repository root. The client selects it in `games/bard_battle/src/app/application.cpp` (`kDefaultMapName`); the server default and `--map` option are in `games/bard_battle/src/server/server_config.h` and `.cpp`.
