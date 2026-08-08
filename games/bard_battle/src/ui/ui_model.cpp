#include "ui/ui_model.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>

namespace ui {
bool valid_player_name(std::string_view name){if(name.empty()||name.size()>24)return false;bool non_space=false;for(unsigned char c:name){if(c<0x20||c>0x7e)return false;if(c!=' ')non_space=true;}return non_space;}
bool parse_endpoint(std::string_view text,std::string & host,std::uint16_t & port){host.clear();port=net::kServerPort;if(text.empty()||text.find_first_of(" \t\r\n/:")!=std::string_view::npos)return false;host=std::string(text);return true;}
std::string truncate_ellipsis(std::string_view value,std::size_t limit){if(value.size()<=limit)return std::string(value);if(limit<=3)return std::string(limit,'.');return std::string(value.substr(0,limit-3))+"...";}
ScoreboardLayout scoreboard_layout(std::size_t count,int width,int height){ScoreboardLayout out{};out.panel_width=std::clamp(width*0.78f,320.0f,1100.0f);out.panel_height=std::clamp(height*0.84f,240.0f,900.0f);const float available=std::max(80.0f,out.panel_height-100.0f);out.row_height=std::clamp(available/std::max<std::size_t>(count,1),11.0f,24.0f);out.rows_per_column=std::max(1u,static_cast<unsigned>(available/out.row_height));if(count>out.rows_per_column){out.columns=2;out.rows_per_column=static_cast<unsigned>((count+1)/2);out.row_height=std::clamp(available/out.rows_per_column,8.0f,20.0f);}out.font_scale=std::clamp(out.row_height/20.0f,0.42f,1.0f);const float column_width=(out.panel_width-36)/out.columns;out.name_characters=static_cast<std::size_t>(std::max(4.0f,(column_width-205*out.font_scale)/(8*out.font_scale)));return out;}
static bool better(const net::PlayerState&a,const net::PlayerState&b){if(a.round_kills!=b.round_kills)return a.round_kills>b.round_kills;if(a.round_deaths!=b.round_deaths)return a.round_deaths<b.round_deaths;if(a.name!=b.name)return a.name<b.name;return a.id<b.id;}
std::vector<net::PlayerState> sort_ffa(std::span<const net::PlayerState> players){std::vector<net::PlayerState> result(players.begin(),players.end());std::sort(result.begin(),result.end(),better);return result;}
std::vector<TeamGroup> sort_teams(std::span<const net::PlayerState> players,std::uint8_t count){std::vector<TeamGroup> groups;for(net::TeamId team=1;team<=count;++team)groups.push_back({team,0,{}});for(const auto&p:players)if(p.team>=1&&p.team<=count){auto&g=groups[p.team-1];g.kills+=p.round_kills;g.players.push_back(p);}for(auto&g:groups)std::sort(g.players.begin(),g.players.end(),better);std::sort(groups.begin(),groups.end(),[](const auto&a,const auto&b){return a.kills!=b.kills?a.kills>b.kills:a.team<b.team;});return groups;}
} // namespace ui
