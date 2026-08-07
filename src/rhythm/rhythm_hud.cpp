#include "rhythm/rhythm_hud.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace {
constexpr std::int64_t kHitWindowMs=80;
constexpr std::int64_t kTravelMs=1000;

void add_rect(RenderBatch & batch, float x, float y, float width, float height, float r, float g, float b, float a=1.0f) {
    InstanceData instance{};instance.world_pos={x,y};instance.size={width,height};instance.flags=kInstanceFlagScreenSpace|kInstanceFlagSolidColor|kInstanceFlagIgnoreFog;instance.opacity=1.0f;instance.color[0]=r;instance.color[1]=g;instance.color[2]=b;instance.color[3]=a;batch.instances.push_back(instance);++batch.debug_instance_count;
}
}

void RhythmHud::schedule(const net::SongSchedule & schedule, std::uint64_t local_start_us) {
    schedule_=schedule;local_start_us_=local_start_us;note_states_.assign(schedule.note_times_ms.size(),0);last_result_={};feedback_time_us_=0;scheduled_=true;pending_note_=UINT32_MAX;
}

void RhythmHud::predict_hit(std::uint64_t now_us, std::int16_t calibration_ms) {
    if(!scheduled_)return;
    const auto relative=(static_cast<std::int64_t>(now_us)-static_cast<std::int64_t>(local_start_us_))/1000-calibration_ms;
    std::size_t best=note_states_.size();std::int64_t distance=kHitWindowMs+1;
    for(std::size_t i=0;i<note_states_.size();++i){if(note_states_[i]!=0)continue;const auto d=std::llabs(relative-static_cast<std::int64_t>(schedule_.note_times_ms[i]));if(d<distance){distance=d;best=i;}}
    if(best<note_states_.size()&&distance<=kHitWindowMs){note_states_[best]=1;pending_note_=static_cast<std::uint32_t>(best);}
}

void RhythmHud::apply_result(const net::RhythmResult & result, std::uint64_t now_us) {
    last_result_=result;feedback_time_us_=now_us;
    if(result.overstrum&&pending_note_<note_states_.size())note_states_[pending_note_]=0;
    if(result.note_index<note_states_.size())note_states_[result.note_index]=2;
    pending_note_=UINT32_MAX;
}

void RhythmHud::append_instances(RenderBatch & batch, int width, int height, std::uint64_t now_us) const {
    if(width<=0||height<=0)return;
    const float center=width*0.5f;const float lane_y=static_cast<float>(height)-70.0f;
    if(batch.debug_instance_count==0)batch.debug_instance_offset=static_cast<std::uint32_t>(batch.instances.size());
    add_rect(batch,center,lane_y,std::max(100.0f,static_cast<float>(width)-64.0f),76.0f,0.055f,0.065f,0.09f,0.92f);
    add_rect(batch,center,lane_y,8.0f,92.0f,0.9f,0.9f,0.95f);
    add_rect(batch,center,lane_y,38.0f,50.0f,0.16f,0.22f,0.28f);
    if(feedback_time_us_!=0&&now_us-feedback_time_us_<=750000){
        if(last_result_.grade==net::RhythmGrade::perfect)add_rect(batch,center,lane_y-55.0f,120.0f,14.0f,0.2f,1.0f,0.35f);
        else if(last_result_.grade==net::RhythmGrade::good)add_rect(batch,center,lane_y-55.0f,120.0f,14.0f,1.0f,0.82f,0.15f);
        else add_rect(batch,center,lane_y-55.0f,120.0f,14.0f,1.0f,0.2f,0.2f);
    }
    if(!scheduled_)return;
    const auto relative=(static_cast<std::int64_t>(now_us)-static_cast<std::int64_t>(local_start_us_))/1000;const float spawn=static_cast<float>(width)-48.0f;
    for(std::size_t i=0;i<schedule_.note_times_ms.size();++i){if(note_states_[i]!=0)continue;const auto until=static_cast<std::int64_t>(schedule_.note_times_ms[i])-relative;if(until>kTravelMs||until<-250)continue;const float progress=1.0f-static_cast<float>(until)/kTravelMs;const float x=spawn+(center-spawn)*progress;float alpha=until< -80?std::max(0.0f,1.0f-static_cast<float>(-until-80)/170.0f):1.0f;add_rect(batch,x,lane_y,24.0f,42.0f,0.15f,0.85f,1.0f,alpha);}
}

std::string RhythmHud::feedback(std::uint64_t now_us) const {
    if(feedback_time_us_==0||now_us-feedback_time_us_>750000)return {};
    std::ostringstream out;if(last_result_.overstrum)out<<"OVERSTRUM";else out<<net::grade_name(last_result_.grade);
    if(last_result_.grade!=net::RhythmGrade::miss){out<<" | "<<std::abs(last_result_.offset_ms)<<" ms "<<(last_result_.offset_ms<0?"EARLY":"LATE");}
    else if(!last_result_.overstrum)out<<" | MISSED";
    out<<" | Combo "<<last_result_.combo;return out.str();
}
