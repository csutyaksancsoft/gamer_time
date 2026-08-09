#include "render/entity_animations.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

int main(){
    const auto require=[](bool c){if(!c)throw std::runtime_error("entity animation assertion failed");};
    const auto root=std::filesystem::temp_directory_path()/"bard_entity_animation_tests";
    std::filesystem::remove_all(root);std::filesystem::create_directories(root);
    {std::ofstream tsx(root/"player.tsx");tsx<<R"(<tileset tilewidth="12" tileheight="20" tilecount="3" columns="3"><properties><property name="bard.role" value="player_1"/><property name="bard.world_width" value="18"/><property name="bard.world_height" value="30"/></properties><image source="player.png" width="36" height="20"/><tile id="0"><animation><frame tileid="0" duration="50"/><frame tileid="2" duration="100"/><frame tileid="1" duration="150"/></animation></tile></tileset>)";}
    auto catalog=entity_animation::Catalog::load(root);const auto*s=catalog.find(entity_animation::Role::player_1);require(s&&s->frames.size()==3&&s->frames[1].tile_id==2);require(s->world_size.x==18&&s->world_size.y==30);require(entity_animation::frame_index(*s,49000,true)==0);require(entity_animation::frame_index(*s,50000,true)==1);require(entity_animation::frame_index(*s,299000,true)==2);require(entity_animation::frame_index(*s,300000,true)==0);require(entity_animation::frame_index(*s,999000,false)==2);
    require(entity_animation::player_variant(4,0,true)==4);require(entity_animation::player_variant(0,0x12345678,false)==entity_animation::player_variant(0,0x12345678,false));
    {std::ofstream duplicate(root/"duplicate.tsx");duplicate<<R"(<tileset tilewidth="1" tileheight="1" tilecount="1" columns="1"><properties><property name="bard.role" value="player_1"/></properties><image source="x.png"/><tile id="0"><animation><frame tileid="0" duration="1"/></animation></tile></tileset>)";}
    auto duplicated=entity_animation::Catalog::load(root);require(duplicated.find(entity_animation::Role::player_1));require(!duplicated.warnings().empty());
    std::filesystem::remove_all(root);
}
