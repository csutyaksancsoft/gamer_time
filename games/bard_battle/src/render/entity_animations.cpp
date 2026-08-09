#include "render/entity_animations.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <regex>
#include <sstream>

namespace entity_animation {
namespace {
std::string read(const std::filesystem::path&p){std::ifstream in(p);if(!in)throw std::runtime_error("cannot open file");return {std::istreambuf_iterator<char>(in),{}};}
std::string attr(const std::string&s,const char*n){std::smatch m;const std::regex re(std::string(n)+R"(\s*=\s*["']([^"']*)["'])");return std::regex_search(s,m,re)?m[1].str():std::string{};}
std::uint32_t number(const std::string&s,const char*n){const auto v=attr(s,n);if(v.empty())throw std::runtime_error(std::string("missing ")+n);const auto x=std::stoull(v);if(x>UINT32_MAX)throw std::runtime_error("numeric property overflow");return static_cast<std::uint32_t>(x);}
std::optional<Role> role(std::string_view s){static constexpr std::array names{"player_1","player_2","player_3","player_4","projectile","shield","melee"};for(std::size_t i=0;i<names.size();++i)if(s==names[i])return static_cast<Role>(i);return {};}
std::string property(const std::string&s,const std::string&name){const std::regex re(R"(<property\b[^>]*name\s*=\s*["'])"+name+R"(["'][^>]*(?:value\s*=\s*["']([^"']*)["'][^>]*/>|>([^<]*)</property>))");std::smatch m;return std::regex_search(s,m,re)?(m[1].matched?m[1].str():m[2].str()):std::string{};}
float positive_property(const std::string&s,const char*n,float fallback){const auto v=property(s,n);if(v.empty())return fallback;const float x=std::stof(v);if(!std::isfinite(x)||x<=0)throw std::runtime_error(std::string("invalid ")+n);return x;}
}

Catalog Catalog::load(const std::filesystem::path&dir){Catalog out;if(!std::filesystem::exists(dir)){out.warnings_.push_back("entity animation directory is missing: "+dir.string());return out;}for(const auto&e:std::filesystem::directory_iterator(dir)){if(!e.is_regular_file()||e.path().extension()!=".tsx")continue;try{const auto xml=read(e.path());const auto r=role(property(xml,"bard.role"));if(!r)throw std::runtime_error("missing or invalid bard.role");const auto slot=static_cast<std::size_t>(*r);if(out.strips_[slot]){out.warnings_.push_back("duplicate entity role ignored: "+e.path().string());continue;}Strip s{};s.role=*r;s.tsx_path=e.path();s.source_width=number(xml,"tilewidth");s.source_height=number(xml,"tileheight");s.tile_count=number(xml,"tilecount");s.columns=number(xml,"columns");s.margin=attr(xml,"margin").empty()?0:number(xml,"margin");s.spacing=attr(xml,"spacing").empty()?0:number(xml,"spacing");if(!s.source_width||!s.source_height||!s.tile_count||s.columns!=s.tile_count)throw std::runtime_error("sheet must be one non-empty horizontal row");std::smatch im;if(!std::regex_search(xml,im,std::regex(R"(<image\b[^>]*source\s*=\s*["']([^"']+)["'])")))throw std::runtime_error("missing image source");s.image_path=e.path().parent_path()/im[1].str();s.world_size={positive_property(xml,"bard.world_width",*r==Role::projectile?18.0f:*r==Role::shield?40.0f:*r==Role::melee?48.0f:16.0f),positive_property(xml,"bard.world_height",*r==Role::projectile?6.0f:*r==Role::shield?40.0f:*r==Role::melee?48.0f:16.0f)};const std::regex fr(R"(<frame\b[^>]*tileid\s*=\s*["']([0-9]+)["'][^>]*duration\s*=\s*["']([0-9]+)["'][^>]*/>)");for(auto i=std::sregex_iterator(xml.begin(),xml.end(),fr);i!=std::sregex_iterator();++i){Frame f{static_cast<std::uint32_t>(std::stoul((*i)[1].str())),static_cast<std::uint32_t>(std::stoul((*i)[2].str())),0};if(f.tile_id>=s.tile_count||!f.duration_ms)throw std::runtime_error("invalid animation frame");s.frames.push_back(f);}if(s.frames.empty())throw std::runtime_error("missing tile animation");out.strips_[slot]=std::move(s);}catch(const std::exception&x){out.warnings_.push_back("invalid entity TSX "+e.path().string()+": "+x.what());}}for(std::size_t i=0;i<out.strips_.size();++i)if(!out.strips_[i])out.warnings_.push_back("missing entity animation role "+std::to_string(i));return out;}
const Strip*Catalog::find(Role r)const{const auto&v=strips_[static_cast<std::size_t>(r)];return v?&*v:nullptr;}

std::uint32_t frame_index(const Strip&s,std::uint64_t us,bool loop){if(s.frames.empty())return 0;std::uint64_t total=0;for(const auto&f:s.frames)total+=f.duration_ms;if(!total)return 0;std::uint64_t ms=us/1000;if(loop)ms%=total;else if(ms>=total)return static_cast<std::uint32_t>(s.frames.size()-1);std::uint64_t end=0;for(std::uint32_t i=0;i<s.frames.size();++i){end+=s.frames[i].duration_ms;if(ms<end)return i;}return static_cast<std::uint32_t>(s.frames.size()-1);}
std::uint8_t player_variant(std::uint8_t team,std::uint32_t color,bool teams){if(teams&&team>=1&&team<=4)return team;std::uint32_t x=color;x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return static_cast<std::uint8_t>(x%4+1);}
} // namespace entity_animation
