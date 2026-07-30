#pragma once

#include "CameraConstants.hpp"
#include "core/KeyObserver.hpp"
#include "core/MouseObserver.hpp"
#include "entt-main/src/entt/entt.hpp"

class KeyHandling;
struct KinematicBody;

class KinematicController : public KeyObserver, public MouseObserver {
    std::unordered_map<int, std::function<void(double)>> cameraKeyBindings;

   public:
    KinematicController();
    ~KinematicController();

    entt::entity activeKinematicBody;

    void update(double deltaTime);
    void insertCallbackFunction(int key, void (KinematicController::*function)(double));
    void setCameraKeyBindings();
    void onKeyEvent(int key, int scancode, int action, int mods) override;
    void onContinuousInput(double deltaTime, const KeyHandling& keyHandler) override;
    void mouseCursorEvent(double xPosIn, double yposIn);
    void moveBackward(double deltaTime);
    void moveForward(double deltaTime);
    void moveLeft(double deltaTime);
    void moveRight(double deltaTime);
    void setYaw(float radians);
    void setPitch(float radians);
    glm::vec3 getMovementVector(const glm::vec3& freeVector, const glm::vec3& fixedVector);
    glm::vec3 lastValidMovementDirection;
    bool firstMouse;
    void setMaxYawRate(float value) { CameraConstants::maxYawRate = glm::radians(value); }
    void setMaxPitchRate(float value) { CameraConstants::maxPitchRate = glm::radians(value); }

    KinematicBody& getActiveKinematicBody();

    void DEBUGVARIABLES_CAMERA();

   private:
};
