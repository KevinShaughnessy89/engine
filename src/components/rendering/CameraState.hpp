#pragma once

#include "rendering/CameraMode.hpp"

// Camera-controller bookkeeping only. Movement/collision state lives in KinematicBody, and
// view/orientation state lives in ViewState -- both attached alongside this on the camera entity.
struct CameraState {
    glm::vec3 position{0.0f};
    CameraMode cameraMode = CameraMode::FREE;
    bool modeChangedFlag = false;
};
