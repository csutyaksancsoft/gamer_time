#pragma once

#include "net/protocol.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <span>
#include <vector>

namespace ui {

struct Rect { float x=0,y=0,width=0,height=0; bool contains(float px,float py) const{return px>=x&&py>=y&&px<x+width&&py<y+height;} };
struct Color { float r=1,g=1,b=1,a=1; };
enum class Align : std::uint8_t { left, center, right };
struct ScreenQuad { Rect bounds{}; std::uint16_t sprite=0; Color color{}; bool use_atlas=true; };
struct TextRun { std::string text; Rect bounds{}; float scale=1; Color color{}; Align alignment=Align::left; bool clip=true; };
enum class Action : std::uint8_t { none, connect, vote_teams, vote_ffa, team_red, team_blue, team_green, team_gold };
struct HitRegion { Rect bounds{}; Action action=Action::none; bool enabled=true; };
struct DrawList { std::vector<ScreenQuad> quads; std::vector<TextRun> text; std::vector<HitRegion> hits; void clear(){quads.clear();text.clear();hits.clear();} };

struct Theme {
    std::uint16_t panel_sprite=0,input_sprite=1,button_sprite=2,button_hover_sprite=3,button_pressed_sprite=4,selection_sprite=5;
    std::array<std::uint16_t,4> team_badges{6,7,8,9};
    Color panel{0.04f,0.06f,0.10f,0.94f}, input{0.08f,0.11f,0.17f,1}, button{0.16f,0.22f,0.34f,1}, selection{0.95f,0.75f,0.18f,1}, text{0.94f,0.96f,1,1};
    float spacing=12,padding=18,button_height=42;
};

bool valid_player_name(std::string_view name);
bool parse_endpoint(std::string_view text, std::string & host, std::uint16_t & port);
std::string truncate_ellipsis(std::string_view text, std::size_t max_chars);

struct ScoreboardLayout { unsigned columns=1; unsigned rows_per_column=0; float row_height=20; float font_scale=1; float panel_width=0; float panel_height=0; std::size_t name_characters=24; };
ScoreboardLayout scoreboard_layout(std::size_t player_count, int window_width, int window_height);
std::vector<net::PlayerState> sort_ffa(std::span<const net::PlayerState> players);
struct TeamGroup { net::TeamId team=0; std::uint32_t kills=0; std::vector<net::PlayerState> players; };
std::vector<TeamGroup> sort_teams(std::span<const net::PlayerState> players, std::uint8_t team_count);

} // namespace ui
