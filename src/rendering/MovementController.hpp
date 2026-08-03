#pragma once

#include "CameraConstants.hpp"
#include "components/physics/KinematicBody.hpp"
#include "core/KeyObserver.hpp"
#include "core/MouseObserver.hpp"

class KeyHandling;

// Owns WASD + mouse-look input. WASD drives the camera's deltaPos in FREE or the character's
// deltaPos in FIXED, never both. Mouse-look always turns the camera; it only turns the character
// (via its own decoupled yaw) while FIXED.
class MovementController : public KeyObserver, public MouseObserver {
    std::unordered_map<int, std::function<void(double)>> movementKeyBindings;
    std::unordered_map<int, std::function<void()>> keyEventBindings;

   public:
    MovementController();
    ~MovementController();

    void onKeyEvent(int key, int scancode, int action, int mods) override;
    void onContinuousInput(double deltaTime, const KeyHandling& keyHandler) override;
    void mouseCursorEvent(double xPosIn, double yPosIn) override;

    void moveForward(double deltaTime);
    void moveBackward(double deltaTime);
    void moveLeft(double deltaTime);
    void moveRight(double deltaTime);
    void toggleMovementState();
    void toggleFreelook();
    void setYaw(float radians);
    void setPitch(float radians);

    bool firstMouse = true;

   private:
    void insertCallbackFunction(int key, void (MovementController::*function)(double));
    void insertKeyEventCallbackFunction(int key, void (MovementController::*function)());
    void setMovementKeyBindings();
    void setKeyEventBindings();
    void applyMovement(const glm::vec3& freeVector, double deltaTime);
    void applyDirection(MovementDirection direction);
};
