#include "platform/camera_controller.h"

#include <algorithm>

void CameraController::update(const InputState & input, float dt_seconds) {
    constexpr float kZoomStep = 0.12f;
    constexpr float kMinZoom = 0.35f;
    constexpr float kMaxZoom = 3.0f;

    (void)dt_seconds;
    state_.zoom = std::clamp(state_.zoom + input.wheel_delta * kZoomStep, kMinZoom, kMaxZoom);
}
