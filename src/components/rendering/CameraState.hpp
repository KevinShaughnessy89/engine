#pragma once

#include "entt-main/src/entt/entt.hpp"
#include "rendering/CameraMode.hpp"

// Camera-controller bookkeeping only. Position/movement/orientation state lives in ViewState,
// attached alongside this on the camera entity.
struct CameraState {
    CameraMode cameraMode = CameraMode::FREE;
    bool modeChangedFlag = false;
    entt::entity targetEntity = entt::null;  // followed in FIXED mode, e.g. CharacterController::activeCharacter
};
