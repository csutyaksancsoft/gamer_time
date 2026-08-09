#include "render/entity_animations.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

int main(){
    const auto require=[](bool c){if(!c)throw std::runtime_error("entity animation assertion failed");};
    const auto root=std::filesystem::temp_directory_path()/"bard_entity_animation_tests";
    std::filesystem::remove_all(root);std::filesystem::create_directories(root/"players"/"green");
    {std::ofstream tsx(root/"players"/"green"/"player.tsx");tsx<<R"(<tileset tilewidth="12" tileheight="20" tilecount="3" columns="3"><properties><property name="bard.player" value="1"/><property name="bard.animation" value="idle"/><property name="bard.world_width" value="18"/><property name="bard.world_height" value="30"/></properties><image source="player.png" width="36" height="20"/><tile id="0"><animation><frame tileid="0" duration="50"/><frame tileid="2" duration="100"/><frame tileid="1" duration="150"/></animation></tile></tileset>)";}
    auto catalog=entity_animation::Catalog::load(root);const auto*s=catalog.find_player(1,entity_animation::PlayerAnimation::idle);require(s&&s->frames.size()==3&&s->frames[1].tile_id==2);require(catalog.find_player(4,entity_animation::PlayerAnimation::attack)==s);require(s->world_size.x==18&&s->world_size.y==30);require(entity_animation::frame_index(*s,49000,true)==0);require(entity_animation::frame_index(*s,50000,true)==1);require(entity_animation::frame_index(*s,299000,true)==2);require(entity_animation::frame_index(*s,300000,true)==0);require(entity_animation::frame_index(*s,999000,false)==2);
    require(entity_animation::player_folder(1,0,false)==1);require(entity_animation::player_folder(2,0,false)==2);require(entity_animation::player_folder(3,0,false)==3);require(entity_animation::player_folder(4,0,false)==4);require(entity_animation::player_folder(5,0,false)==1);require(entity_animation::player_folder(0,0,false)==1);
    require(entity_animation::player_folder(1,4,true)==4);require(entity_animation::player_folder(4,2,true)==2);require(entity_animation::player_folder(2,0,true)==2);
    {std::ofstream duplicate(root/"z_duplicate.tsx");duplicate<<R"(<tileset tilewidth="1" tileheight="1" tilecount="1" columns="1"><properties><property name="bard.player" value="1"/><property name="bard.animation" value="idle"/></properties><image source="x.png"/><tile id="0"><animation><frame tileid="0" duration="1"/></animation></tile></tileset>)";}
    auto duplicated=entity_animation::Catalog::load(root);require(duplicated.find_player(1,entity_animation::PlayerAnimation::idle));require(!duplicated.warnings().empty());
    std::filesystem::remove_all(root);
}
