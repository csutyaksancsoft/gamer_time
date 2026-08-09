#include "server/spawn_zones.h"

#include "core/error.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace server {
namespace {

constexpr float kEpsilon = 0.0001f;

float polygon_area(std::span<const Vec2f> points) {
    float twice = 0.0f;
    for (std::size_t i=0; i<points.size(); ++i) {
        const auto & a=points[i]; const auto & b=points[(i+1)%points.size()];
        twice += a.x*b.y-b.x*a.y;
    }
    return std::abs(twice)*0.5f;
}

bool point_in_polygon(std::span<const Vec2f> polygon, Vec2f point) {
    bool inside=false;
    for(std::size_t i=0,j=polygon.size()-1;i<polygon.size();j=i++) {
        const auto & a=polygon[i]; const auto & b=polygon[j];
        if(((a.y>point.y)!=(b.y>point.y)) &&
           point.x < (b.x-a.x)*(point.y-a.y)/(b.y-a.y)+a.x) inside=!inside;
    }
    return inside;
}

bool finite(Vec2f p) { return std::isfinite(p.x)&&std::isfinite(p.y); }

std::string object_label(const std::string & layer, std::uint32_t id) {
    return "respawn layer '"+layer+"' object "+std::to_string(id);
}

float min_player_distance_squared(Vec2f p, std::span<const SpawnOccupant> occupants) {
    float result=std::numeric_limits<float>::infinity();
    for(const auto & other:occupants) if(other.living) result=std::min(result,length_squared(p-other.position));
    return result;
}

} // namespace

SpawnZones::SpawnZones(const MapWorld & map, const CollisionWorld & collision):collision_(&collision) {
    auto load=[&](const std::string & name, std::vector<Region> & destination) {
        std::vector<const ObjectLayer*> matches;
        for(const auto & layer:map.object_layers()) if(layer.name==name) matches.push_back(&layer);
        if(matches.empty()) fail("Missing required respawn Object Layer '"+name+"'");
        if(matches.size()!=1) fail("Duplicate required respawn Object Layer '"+name+"'");
        for(const auto & object:matches.front()->objects) {
            Region region{}; region.layer=name; region.object_id=object.id;
            if(object.is_tile||object.shape==MapObjectShape::Point||object.shape==MapObjectShape::None) continue;
            if(object.shape==MapObjectShape::Rectangle) {
                if(object.rotation!=0.0f||!finite(object.position)||!finite(object.size)||object.size.x<=0.0f||object.size.y<=0.0f) continue;
                region.rectangle=true;
                region.points={{object.position.x,object.position.y-object.size.y},
                               {object.position.x+object.size.x,object.position.y-object.size.y},
                               {object.position.x+object.size.x,object.position.y},object.position};
                region.area=object.size.x*object.size.y;
            } else if(object.shape==MapObjectShape::Polygon) {
                region.points=object.polygon.points;
                if(region.points.size()<3||std::any_of(region.points.begin(),region.points.end(),[](Vec2f p){return !finite(p);})) continue;
                region.area=polygon_area(region.points);
                if(region.area<=kEpsilon) continue;
            } else continue;

            Vec2f min=region.points.front(),max=min;
            for(Vec2f p:region.points){min.x=std::min(min.x,p.x);min.y=std::min(min.y,p.y);max.x=std::max(max.x,p.x);max.y=std::max(max.y,p.y);}
            // A fixed dense scan makes startup validation deterministic and also gives
            // runtime sampling guaranteed-safe fallback candidates.
            constexpr int divisions=32;
            for(int y=0;y<divisions;++y) for(int x=0;x<divisions;++x) {
                Vec2f p{min.x+(max.x-min.x)*(x+0.5f)/divisions,min.y+(max.y-min.y)*(y+0.5f)/divisions};
                if(valid_position(region,p)) region.valid_anchors.push_back(p);
            }
            Vec2f centroid{};for(Vec2f p:region.points)centroid=centroid+p;centroid=centroid/static_cast<float>(region.points.size());
            if(valid_position(region,centroid)) region.valid_anchors.push_back(centroid);
            if(region.valid_anchors.empty()) fail(object_label(name,object.id)+" has no statically usable player-collision-free location");
            destination.push_back(std::move(region));
        }
        if(destination.empty()) fail("Required respawn Object Layer '"+name+"' contains no usable rectangle or polygon");
    };
    for(std::size_t i=0;i<4;++i) {
        load("respawn_zone_"+std::to_string(i+1),zones_[i]);
    }
}

bool SpawnZones::valid_position(const Region & region, Vec2f point) const {
    const std::array<Vec2f,4> corners{{{point.x-kPlayerRadius,point.y-kPlayerRadius},{point.x+kPlayerRadius,point.y-kPlayerRadius},{point.x+kPlayerRadius,point.y+kPlayerRadius},{point.x-kPlayerRadius,point.y+kPlayerRadius}}};
    for(Vec2f corner:corners) if(!point_in_polygon(region.points,corner)) return false;
    CollisionBounds bounds{{point.x-kPlayerRadius,point.y-kPlayerRadius},{point.x+kPlayerRadius,point.y+kPlayerRadius}};
    return collision_->query_bounds(CollisionChannel::Player,bounds).empty();
}

Vec2f SpawnZones::random_point(const Region & region, std::mt19937 & random) const {
    std::uniform_real_distribution<float> unit(0.0f,1.0f);
    if(region.rectangle) {
        const auto & a=region.points[0]; const auto & c=region.points[2];
        return {a.x+kPlayerRadius+unit(random)*(c.x-a.x-2*kPlayerRadius),a.y+kPlayerRadius+unit(random)*(c.y-a.y-2*kPlayerRadius)};
    }
    Vec2f min=region.points.front(),max=min;for(Vec2f p:region.points){min.x=std::min(min.x,p.x);min.y=std::min(min.y,p.y);max.x=std::max(max.x,p.x);max.y=std::max(max.y,p.y);}
    for(int attempt=0;attempt<128;++attempt) {
        Vec2f p{min.x+unit(random)*(max.x-min.x),min.y+unit(random)*(max.y-min.y)};
        if(point_in_polygon(region.points,p)) return p;
    }
    return region.valid_anchors.front();
}

Vec2f SpawnZones::sample(net::GameMode mode,net::TeamId team,std::mt19937 & random,std::span<const SpawnOccupant> occupants) const {
    std::vector<const Region*> regions;
    if(mode==net::GameMode::ffa) for(const auto & group:zones_) for(const auto & region:group) regions.push_back(&region);
    else {
        if(team<net::kFirstTeam||team>net::kMaxTeam) fail("Cannot select respawn zone for invalid team "+std::to_string(team));
        for(const auto & region:zones_[team-net::kFirstTeam]) regions.push_back(&region);
    }
    float total=0.0f;for(const auto *r:regions)total+=r->area;
    std::uniform_real_distribution<float> choose(0.0f,total);
    Vec2f best{};float best_distance=-1.0f;bool have_best=false;
    constexpr int attempts=256;
    for(int attempt=0;attempt<attempts;++attempt) {
        float selection=choose(random);const Region *region=regions.back();
        for(const auto *candidate:regions){selection-=candidate->area;if(selection<=0.0f){region=candidate;break;}}
        Vec2f p=random_point(*region,random);if(!valid_position(*region,p))continue;
        const float distance=min_player_distance_squared(p,occupants);
        if(distance>=kPlayerSeparation*kPlayerSeparation)return p;
        if(!have_best||distance>best_distance){best=p;best_distance=distance;have_best=true;}
    }
    for(const Region * region:regions) for(Vec2f p:region->valid_anchors) {
        const float distance=min_player_distance_squared(p,occupants);
        if(distance>=kPlayerSeparation*kPlayerSeparation)return p;
        if(!have_best||distance>best_distance){best=p;best_distance=distance;have_best=true;}
    }
    if(!have_best) fail("Respawn zones unexpectedly produced no collision-free candidate");
    return best;
}

} // namespace server
