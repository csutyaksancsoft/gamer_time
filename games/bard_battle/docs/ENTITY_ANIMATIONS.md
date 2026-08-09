# Entity animation assets

Put standalone `.tsx` files in `assets/tiled_projects/entities/`. Each tileset must describe one horizontal row of equally sized frames and include one animation. Transparent PNG pixels remain transparent in game.

Every file needs a string property named `bard.role`. Valid values are `player_1`, `player_2`, `player_3`, `player_4`, `projectile`, `shield`, and `melee`. Optional positive float properties `bard.world_width` and `bard.world_height` set the rendered world-space size; omitting them keeps the existing gameplay visual size.

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

Player and horizontal movement art faces right. Projectile and melee art must also face right; the client rotates those sheets to the server-authoritative attack angle. The first player frame is the idle pose. Empty/transparent frames are allowed and retain their duration.

Malformed files, duplicate roles, and missing roles produce warnings. Their gameplay objects remain visible using the existing player placeholders, projectile bar, shield ring, or melee ring.
