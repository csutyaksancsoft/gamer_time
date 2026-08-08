#include "game/match_rules.h"
#include "ui/ui_model.h"
#include <stdexcept>
#include <iostream>

int main(){auto check=[](bool v){if(!v)throw std::runtime_error("match/ui assertion failed");};
 game::ModeVote vote;const net::PlayerId ids[]={1,2,3};vote.begin(ids,100);check(!vote.complete(101));check(vote.submit(1,net::VoteChoice::teams));check(!vote.submit(1,net::VoteChoice::ffa));check(vote.submit(2,net::VoteChoice::ffa));vote.disconnect(3);check(vote.complete(102));check(vote.result()==net::GameMode::ffa);
 vote.begin(ids,100);check(vote.complete(100+game::ModeVote::duration_us));check(vote.result()==net::GameMode::ffa);check(!vote.submit(9,net::VoteChoice::teams));
 check(!game::projectile_can_hit(net::GameMode::teams,false,1,1));check(game::projectile_can_hit(net::GameMode::teams,true,1,1));check(!game::kill_is_awarded(net::GameMode::teams,2,2));check(game::kill_is_awarded(net::GameMode::teams,1,2));
 std::string host;std::uint16_t port=0;check(ui::valid_player_name("Player One"));check(!ui::valid_player_name("   "));check(ui::parse_endpoint("localhost",host,port)&&port==27020);check(ui::parse_endpoint("10.0.0.2:3000",host,port)&&port==3000);check(!ui::parse_endpoint("host:0",host,port));
 std::vector<net::PlayerState> players(64);for(std::size_t i=0;i<players.size();++i){players[i].id=static_cast<net::PlayerId>(i+1);players[i].name="Player"+std::to_string(i);players[i].round_kills=static_cast<std::uint32_t>(i%5);players[i].round_deaths=static_cast<std::uint32_t>(i%3);players[i].team=static_cast<net::TeamId>(i%4+1);}auto sorted=ui::sort_ffa(players);check(sorted.front().round_kills==4);auto layout=ui::scoreboard_layout(64,1280,720);check(layout.columns==2&&layout.rows_per_column>=32&&layout.font_scale>0);auto groups=ui::sort_teams(players,4);check(groups.size()==4);check(ui::truncate_ellipsis("abcdefgh",6)=="abc...");
 std::cout<<"match/ui tests passed\n";}
