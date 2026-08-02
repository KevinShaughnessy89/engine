#include "MovementController.hpp"

#include <cmath>

#include "CameraController.hpp"
#include "character/CharacterController.hpp"
#include "components/physics/KinematicBody.hpp"
#include "components/rendering/CameraState.hpp"
#include "components/rendering/ViewState.hpp"
#include "core/Config.hpp"
#include "core/Core.hpp"
#include "core/KeyHandling.hpp"
#include "core/PrecompiledHeader.hpp"
#include "imgui.h"

MovementController::MovementController() {
    setMovementKeyBindings();
}

MovementController::~MovementController() {
}

void MovementController::insertCallbackFunction(int key,
                                                void (MovementController::*function)(double)) {
    movementKeyBindings.emplace(
        key, [this, function](double deltaTime) { (this->*function)(deltaTime); });
}

void MovementController::setMovementKeyBindings() {
    insertCallbackFunction(GLFW_KEY_W, &MovementController::moveForward);
    insertCallbackFunction(GLFW_KEY_A, &MovementController::moveLeft);
    insertCallbackFunction(GLFW_KEY_S, &MovementController::moveBackward);
    insertCallbackFunction(GLFW_KEY_D, &MovementController::moveRight);
}

void MovementController::onKeyEvent(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        CameraController::toggleFreelook();
    }
}

void MovementController::onContinuousInput(double deltaTime, const KeyHandling& keyHandler) {
    for (const auto& [key, func] : movementKeyBindings) {
        if (keyHandler.isKeyPressed(key)) {
            func(deltaTime);
        }
    }
}

double movementLastX = Config::SCREEN_HEIGHT / 2.0;
double movementLastY = Config::SCREEN_WIDTH / 2.0;
void MovementController::mouseCursorEvent(double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        movementLastX = xpos;
        movementLastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - movementLastX;
    float yOffset = movementLastY - ypos;  // reversed for typical "screen up is positive pitch"

    movementLastX = xpos;
    movementLastY = ypos;

    if (ImGui::GetIO().WantCaptureMouse) return;

    setYaw(glm::radians(xOffset));
    setPitch(glm::radians(yOffset));
}

float movementWrapAngle(float angle) {
    const float TWO_PI = glm::two_pi<float>();
    angle = fmod(angle, TWO_PI);
    if (angle < 0.0f) angle += TWO_PI;
    return angle;
}

void MovementController::setPitch(float radians) {
    auto& view = Registry.get<ViewState>(CameraController::activeCamera);
    float delta = radians * CameraConstants::mouseSensitivity;
    delta = glm::clamp(delta, -glm::radians(CameraConstants::maxPitchRate),
                       glm::radians(CameraConstants::maxPitchRate));
    view.pitch += delta;
    view.pitch = glm::clamp(view.pitch, glm::radians(-89.0f), glm::radians(89.0f));
}

// Always turns the camera. Only turns the character's body when it's actually the thing being
// controlled (FIXED) -- otherwise looking around in FREE would spin the character in place while
// it's sitting there unattended.
void MovementController::setYaw(float radians) {
    auto& view = Registry.get<ViewState>(CameraController::activeCamera);
    float delta = -radians * CameraConstants::mouseSensitivity;
    delta = glm::clamp(delta, -CameraConstants::maxYawRate, CameraConstants::maxYawRate);
    view.yaw += delta;
    view.yaw = movementWrapAngle(view.yaw);
}

// Small sentinel written (not accumulated) whenever a grounded move* fires this frame -- gates
// Walk vs Idle clip selection in CharacterController::update without float-precision pitfalls
// from comparing an accumulated value against zero.
constexpr float movementIntentEpsilon = 0.01f;

// WASD drives exactly one thing depending on mode: the free camera in FREE, or the character's
// body in FIXED -- never both, so the unattended one doesn't drift off on its own. In FIXED,
// position itself now comes entirely from the animation clip's root motion (see
// KinematicController::updateBody), so WASD only signals movement intent for clip selection.
void MovementController::applyMovement(const glm::vec3& freeVector, double deltaTime) {
    auto& camState = Registry.get<CameraState>(CameraController::activeCamera);
    if (camState.cameraMode == CameraMode::FREE) {
        auto& view = Registry.get<ViewState>(CameraController::activeCamera);
        view.deltaPos += freeVector * view.speed * (float)deltaTime;
    } else {
        auto& body = Registry.get<KinematicBody>(CharacterController::activeCharacter);
        body.movementIntent = movementIntentEpsilon;
    }
}

void MovementController::moveForward(double deltaTime) {
    auto& view = Registry.get<ViewState>(CameraController::activeCamera);
    glm::vec3 forward =
        glm::normalize(glm::rotate(view.orientation, CameraConstants::WORLD_FORWARD_AXIS));
    applyMovement(forward, deltaTime);
}

void MovementController::moveBackward(double deltaTime) {
    auto& view = Registry.get<ViewState>(CameraController::activeCamera);
    glm::vec3 forward =
        glm::normalize(glm::rotate(view.orientation, CameraConstants::WORLD_FORWARD_AXIS));
    applyMovement(-forward, deltaTime);
}

void MovementController::moveRight(double deltaTime) {
    auto& view = Registry.get<ViewState>(CameraController::activeCamera);
    glm::vec3 forward =
        glm::normalize(glm::rotate(view.orientation, CameraConstants::WORLD_FORWARD_AXIS));
    glm::vec3 right =
        glm::normalize(glm::rotate(view.orientation, CameraConstants::WORLD_RIGHT_AXIS));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));
    glm::vec3 freeVector = glm::cross(forward, up);
    applyMovement(freeVector, deltaTime);
}

void MovementController::moveLeft(double deltaTime) {
    auto& view = Registry.get<ViewState>(CameraController::activeCamera);
    glm::vec3 forward =
        glm::normalize(glm::rotate(view.orientation, CameraConstants::WORLD_FORWARD_AXIS));
    glm::vec3 right =
        glm::normalize(glm::rotate(view.orientation, CameraConstants::WORLD_RIGHT_AXIS));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));
    glm::vec3 freeVector = glm::cross(forward, up);
    applyMovement(-freeVector, deltaTime);
}
