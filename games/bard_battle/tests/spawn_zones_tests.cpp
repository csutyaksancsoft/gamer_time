#include "server/spawn_zones.h"
#include "assets/tmx_map_loader.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace {

void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}

TmxObjectAsset rectangle(std::uint32_t id,float x,float y,float width=100,float height=100) {
    TmxObjectAsset object{};object.id=id;object.x=x;object.y=y;object.width=width;object.height=height;return object;
}

TmxMapAsset fixture() {
    TmxMapAsset asset{};asset.orientation="orthogonal";asset.render_order="right-down";asset.width=100;asset.height=100;asset.tile_width=10;asset.tile_height=10;
    TmxTilesetAsset tiles{};tiles.first_gid=1;tiles.name="fixture";tiles.tile_width=10;tiles.tile_height=10;tiles.tile_count=1;tiles.columns=1;tiles.image_width=10;tiles.image_height=10;asset.tilesets.push_back(tiles);
    for(std::size_t i=0;i<4;++i) {
        TmxObjectLayerAsset team{};team.id=static_cast<std::uint32_t>(i+1);team.name="respawn_zone_"+std::to_string(i+1);team.visible=i!=2;team.objects.push_back(rectangle(static_cast<std::uint32_t>(i+1),50+i*200,500));
        if(i==3){team.objects.back().has_polygon=true;team.objects.back().polygon.points={{0,0},{100,0},{50,100}};}
        asset.object_layers.push_back(team);asset.layer_order.push_back({TmxLayerType::Object,asset.object_layers.size()-1});
    }
    return asset;
}

server::SpawnZones zones_from(const TmxMapAsset & asset) {
    const auto map=MapWorld::from_tmx(asset);return server::SpawnZones(map,CollisionWorld::from_map(map));
}

bool throws_with(const TmxMapAsset & asset,const std::string &part) {
    try{(void)zones_from(asset);}catch(const std::exception&e){return std::string(e.what()).find(part)!=std::string::npos;}return false;
}

} // namespace

int main() {
    auto asset=fixture();const auto map=MapWorld::from_tmx(asset);const auto collision=CollisionWorld::from_map(map);server::SpawnZones zones(map,collision);std::mt19937 random(1234);
    for(net::TeamId team=1;team<=4;++team) for(int i=0;i<50;++i) {
        const Vec2f p=zones.sample(net::GameMode::teams,team,random);
        require(p.x>-450+(team-1)*200&&p.x<-350+(team-1)*200,"team used wrong layer");
        require(p.y>-100&&p.y<0,"team spawn outside authored region");
    }
    bool quadrants[4]{};for(int i=0;i<400;++i){const Vec2f p=zones.sample(net::GameMode::ffa,1,random);const int index=static_cast<int>((p.x+450)/200);if(index>=0&&index<4)quadrants[index]=true;require(p.y>-100&&p.y<0,"FFA outside region");}for(bool hit:quadrants)require(hit,"FFA did not use all layers");

    auto missing=fixture();missing.object_layers.erase(missing.object_layers.begin());missing.layer_order.clear();for(std::size_t i=0;i<missing.object_layers.size();++i)missing.layer_order.push_back({TmxLayerType::Object,i});require(throws_with(missing,"respawn_zone_1"),"missing layer accepted");
    auto wrong_case=fixture();wrong_case.object_layers[0].name="Respawn_zone_1";require(throws_with(wrong_case,"Missing"),"case-insensitive layer accepted");
    auto duplicate=fixture();duplicate.object_layers.push_back(duplicate.object_layers[0]);duplicate.layer_order.push_back({TmxLayerType::Object,duplicate.object_layers.size()-1});require(throws_with(duplicate,"Duplicate"),"duplicate layer accepted");
    auto rotated=fixture();rotated.object_layers[0].objects[0].rotation=10;require(throws_with(rotated,"contains no usable"),"rotated rectangle accepted");
    auto malformed=fixture();malformed.object_layers[0].objects[0].width=0;malformed.object_layers[0].objects.push_back(rectangle(99,0,0));malformed.object_layers[0].objects.back().is_point=true;require(throws_with(malformed,"contains no usable"),"unsupported objects accepted");

    auto blocked=fixture();TmxObjectLayerAsset wall{};wall.name="collision_player";wall.objects.push_back(rectangle(100,50,500));blocked.object_layers.push_back(wall);blocked.layer_order.push_back({TmxLayerType::Object,blocked.object_layers.size()-1});require(throws_with(blocked,"object 1"),"fully blocked zone accepted");

    auto concave=fixture();TmxObjectLayerAsset surround{};surround.name="collision_player";auto u=rectangle(101,50,500);u.has_polygon=true;u.polygon.points={{0,0},{100,0},{100,100},{90,100},{90,10},{10,10},{10,100},{0,100}};surround.objects.push_back(u);concave.object_layers.push_back(surround);concave.layer_order.push_back({TmxLayerType::Object,concave.object_layers.size()-1});require(!throws_with(concave,"object 1"),"concave polygon AABB incorrectly blocked zone");

    const std::string default_map=std::string(BARD_BATTLE_SOURCE_ROOT)+"/games/bard_battle/assets/tiled_projects/maps/brawlers_ballad.tmx";
    const auto default_world=MapWorld::from_tmx(assets::load_tmx_map(default_map));
    const auto default_collision=CollisionWorld::from_map(default_world);
    server::SpawnZones default_zones(default_world,default_collision);

    std::vector<server::SpawnOccupant> occupants{{{-400,350},true},{ {-200,350},true},{ {0,350},true},{ {200,350},true}};
    const Vec2f crowded=zones.sample(net::GameMode::ffa,1,random,occupants);require(std::isfinite(crowded.x)&&std::isfinite(crowded.y),"crowded fallback failed");
}
