#pragma once

#include "entt-main/src/entt/entt.hpp"
#include "rendering/CameraConstants.hpp"

class CameraController {
   public:
    static entt::entity activeCamera;

    static void update(double deltaTime);
    static void init();
    static void toggleFreelook();

    void setMaxYawRate(float value) { CameraConstants::maxYawRate = glm::radians(value); }
    void setMaxPitchRate(float value) { CameraConstants::maxPitchRate = glm::radians(value); }
};