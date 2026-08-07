#include "rhythm/rhythm_hud.h"
#include "rhythm/rhythm_judgment.h"
#include "rhythm/song_config.h"

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

    const auto chart=load_note_chart(GT_SOURCE_DIR "/assets/audio/MEMECAR-001.chart",145000);
    require(chart.size()==290);require(chart.front()==0);require(chart.back()==144500);

    RhythmHud hud;net::SongSchedule schedule{};schedule.duration_ms=2000;schedule.note_times_ms={1000};
    constexpr std::uint64_t start=1000000;hud.schedule(schedule,start);
    RenderBatch batch{};hud.append_instances(batch,800,600,start);
    require(batch.debug_instance_count==4);require(std::abs(batch.instances.back().world_pos.x-752.0f)<0.01f);
    RenderBatch at_target{};hud.append_instances(at_target,800,600,start+1000000);
    require(std::abs(at_target.instances.back().world_pos.x-400.0f)<0.01f);
    hud.predict_hit(start+1000000,0);RenderBatch hidden{};hud.append_instances(hidden,800,600,start+1000000);require(hidden.debug_instance_count==3);
    net::RhythmResult rejected{};rejected.grade=net::RhythmGrade::miss;rejected.overstrum=true;hud.apply_result(rejected,start+1000010);RenderBatch restored{};hud.append_instances(restored,800,600,start+1000010);require(restored.debug_instance_count==5);require(std::abs(restored.instances.back().world_pos.x-400.0f)<0.01f);
}
