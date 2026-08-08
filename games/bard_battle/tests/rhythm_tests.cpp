#include "rhythm/rhythm_hud.h"
#include "rhythm/rhythm_judgment.h"
#include "rhythm/song_config.h"
#include "platform/camera_controller.h"

#include <cmath>
#include <stdexcept>

int main() {
    const auto require=[](bool condition){if(!condition)throw std::runtime_error("rhythm assertion failed");};
    require(rhythm::grade_offset(0)==net::RhythmGrade::perfect);
    require(rhythm::grade_offset(-35)==net::RhythmGrade::perfect);
    require(rhythm::grade_offset(36)==net::RhythmGrade::good);
    require(rhythm::grade_offset(-80)==net::RhythmGrade::good);
    require(rhythm::grade_offset(81)==net::RhythmGrade::miss);
    require(!rhythm::note_expired(1080,1000));
    require(rhythm::note_expired(1081,1000));
    CameraState camera{{100.0f,-50.0f},2.0f};const Vec2f center=screen_to_world(400,300,800,600,camera);require(std::abs(center.x-100.0f)<0.001f&&std::abs(center.y+50.0f)<0.001f);const Vec2f corner=screen_to_world(600,100,800,600,camera);require(std::abs(corner.x-200.0f)<0.001f&&std::abs(corner.y-50.0f)<0.001f);

    SongConfig config{};config.bpm=120.0f;config.first_beat_ms=0;config.subdivision=1;config.duration_ms=145000;const auto chart=generate_beat_grid(config);
    require(chart.size()==290);require(chart.front()==0);require(chart.back()==144500);
    config.first_beat_ms=125;config.subdivision=2;config.duration_ms=1000;const auto offset_chart=generate_beat_grid(config);
    require(offset_chart.size()==4);require(offset_chart[0]==125);require(offset_chart[1]==375);require(offset_chart[3]==875);

    RhythmHud hud;net::SongSchedule schedule{};schedule.duration_ms=2000;schedule.note_times_ms={1000};
    constexpr std::uint64_t start=1000000;hud.schedule(schedule,start);
    RenderBatch batch{};hud.append_instances(batch,800,600,start);
    require(batch.debug_instance_count==4);require(std::abs(batch.instances.back().world_pos.x-752.0f)<0.01f);
    RenderBatch at_target{};hud.append_instances(at_target,800,600,start+1000000);
    require(std::abs(at_target.instances.back().world_pos.x-400.0f)<0.01f);
    hud.predict_hit(start+1000000,0);RenderBatch hidden{};hud.append_instances(hidden,800,600,start+1000000);require(hidden.debug_instance_count==3);
    net::RhythmResult rejected{};rejected.grade=net::RhythmGrade::miss;rejected.overstrum=true;hud.apply_result(rejected,start+1000010);RenderBatch restored{};hud.append_instances(restored,800,600,start+1000010);require(restored.debug_instance_count==5);require(std::abs(restored.instances.back().world_pos.x-400.0f)<0.01f);
}
