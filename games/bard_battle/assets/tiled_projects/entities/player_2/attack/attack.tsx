<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.10" tiledversion="1.12.2" name="player_2_attack" tilewidth="32" tileheight="32" tilecount="4" columns="4">
 <properties>
  <property name="bard.animation" value="attack"/>
  <property name="bard.player" type="int" value="2"/>
  <property name="bard.world_height" type="float" value="40"/>
  <property name="bard.world_width" type="float" value="40"/>
 </properties>
 <image source="attack.png" width="128" height="32"/>
 <tile id="0">
  <animation>
   <frame tileid="0" duration="100"/>
   <frame tileid="1" duration="100"/>
   <frame tileid="2" duration="100"/>
   <frame tileid="3" duration="100"/>
  </animation>
 </tile>
</tileset>
