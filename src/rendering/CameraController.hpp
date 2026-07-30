#pragma once

#include "CameraMode.hpp"
#include "components/physics/KinematicBody.hpp"
#include "components/rendering/CameraState.hpp"
#include "components/rendering/ViewState.hpp"
#include "entt-main/src/entt/entt.hpp"

class CameraController {
   public:
    static entt::entity activeCamera;

    static void update(double deltaTime);
    static CameraState& getCameraState(entt::entity cameraEntity);
    static KinematicBody& getKinematicBody(entt::entity cameraEntity);
    static ViewState& getViewState(entt::entity cameraEntity);
    static void init();
    static void toggleFreelook();
};