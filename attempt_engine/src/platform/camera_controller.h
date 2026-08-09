#pragma once

#include "core/types.h"
#include "platform/input_state.h"

struct CameraState {
    Vec2f world_center{0.0f, 0.0f};
    float zoom = 1.0f;
};
inline Vec2f screen_to_world(int x,int y,int width,int height,const CameraState & c){const float z=c.zoom>0.0001f?c.zoom:1.0f;return {c.world_center.x+(x-width*0.5f)/z,c.world_center.y+(height*0.5f-y)/z};}

class CameraController {
public:
    void update(const InputState & input, float dt_seconds);
    void follow(Vec2f position) { state_.world_center = position; }

    const CameraState & state() const {
        return state_;
    }

private:
    CameraState state_{};
};
