#include "platform/camera_controller.h"
#include "render/render_safety.h"

#include <algorithm>

void CameraController::update(const InputState & input, float dt_seconds) {
    (void)dt_seconds;
    if (input.wheel_delta != 0.0f) {
        state_.zoom = render_safety::select_zoom_level(state_.zoom, input.wheel_delta);
    }
}
