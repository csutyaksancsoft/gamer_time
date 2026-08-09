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

## Collision

Create Object Layers with these exact, case-sensitive names. Add rectangle objects or polygon objects with at least three points. Object names, types, and custom properties do not select behavior.

| Object Layer | Players | Shots | Vision / observer sound |
|---|:---:|:---:|:---:|
| `collision_full` | Blocked | Blocked | Blocked |
| `collision_shots` | Blocked | Blocked | Passes |
| `collision_player` | Blocked | Passes | Passes |

Point objects, tile objects, zero-sized rectangles, and malformed polygons do not collide. Legacy `collision` or `boundary` layers and `engine.*` properties are ignored. Press **F3** in the client to show every active collision channel, including shapes on hidden layers.

## Respawn zones

Every Bard Battle map must contain exactly one Object Layer named `respawn_zone_1` through `respawn_zone_4`. All four exact, case-sensitive layers are mandatory even when the current match uses fewer teams.

Each layer must contain at least one usable positive-area shape. Supported shapes are non-rotated rectangles and polygons with at least three valid points. Point and tile objects, rotated or zero-sized rectangles, and malformed polygons are ignored; object names, types, custom properties, and layer visibility do not affect spawning. Startup fails if a layer is missing, duplicated, has no supported shape, or has no location where the complete player bounds fit without touching player-channel collision.

FFA spawning combines all four `respawn_zone_N` layers. Team spawning uses only `respawn_zone_N` for team N, so a two-team match uses zones 1 and 2. Multiple shapes in a selected layer form one area and are chosen proportionally by area. Spawn layers may be hidden in Tiled and remain active in gameplay.

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

The default map is `games/bard_battle/assets/tiled_projects/maps/brawlers_ballad.tmx` from the repository root. The client selects it in `games/bard_battle/src/app/application.cpp` (`kDefaultMapName`); the server default and `--map` option are in `games/bard_battle/src/server/server_config.h` and `.cpp`.
