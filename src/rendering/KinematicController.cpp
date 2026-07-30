#include "KinematicController.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "CameraController.hpp"
#include "collision/CollisionManager.hpp"
#include "collision/KinematicResolver.hpp"
#include "collision/ResolvedConstraint.hpp"
#include "collision/shapes/ColliderFactory.hpp"
#include "components/collision/ShapeData.hpp"
#include "components/physics/KinematicBody.hpp"
#include "components/rendering/CameraState.hpp"
#include "components/rendering/ViewState.hpp"
#include "core/Core.hpp"
#include "core/KeyHandling.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "imgui.h"

// Everything that reads/writes KinematicBody: gravity, grounding, collision resolution, and the
// velocity/collider bookkeeping derived from the resolved position. Relies on CameraController::
// update() having already run this frame so view.orientation/forward are current.
void KinematicController::update(double deltaTime) {
    float stopThreshold = 0.01f;
    float lerpFactor = 150.f;

    auto& body = getActiveKinematicBody();
    auto& camState = Registry.get<CameraState>(activeKinematicBody);
    auto& view = Registry.get<ViewState>(activeKinematicBody);

    body.orientation = view.orientation;

    if (camState.cameraMode == CameraMode::FIXED) {
        if (camState.modeChangedFlag) {
            body.isGrounded = false;
            body.wasGrounded = false;
            camState.modeChangedFlag = false;
        }

        if (!body.isGrounded) {
            body.wasGrounded = false;
            body.verticalVelocity += CameraConstants::gravity * (deltaTime / 10);
            body.verticalVelocity =
                std::clamp(body.verticalVelocity, CameraConstants::terminalVelocity, 0.0f);
            body.targetPosition.y = body.position.y + body.verticalVelocity * (deltaTime / 10);
        }

        if (body.isGrounded && body.wasGrounded) {
            body.targetPosition = body.position + body.deltaMove;
        }

        if (body.isGrounded && !body.wasGrounded) {
            body.verticalVelocity = 0.0f;
            body.targetPosition = body.position;
            body.wasGrounded = true;
        }

        CandidateSet candidates = Collision.getCandidates(body.targetPosition);

        std::vector<uint32_t> combinedCandidates;
        for (auto entity : candidates.dynamicCandidates) {
            combinedCandidates.push_back(static_cast<uint32_t>(entity));
        }
        combinedCandidates.insert(combinedCandidates.end(),
                                  candidates.triangleCandidates.begin(),
                                  candidates.triangleCandidates.end());

        ResolvedConstraint resolvedConstraint = KinematicResolver::resolveCollisionConstraint(
            body.targetPosition, combinedCandidates, body, deltaTime);

        body.isGrounded = resolvedConstraint.isGrounded;
        glm::vec3 diff = resolvedConstraint.position - body.position;

        if (glm::dot(diff, diff) > stopThreshold * stopThreshold) {
            float alpha = 1.0f - std::exp(-lerpFactor * deltaTime);
            body.position += diff * alpha;
        } else {
            body.position = resolvedConstraint.position;
        }
    } else if (camState.cameraMode == CameraMode::FREE) {
        body.position += body.deltaMove;
        body.targetPosition = body.position;
        if (camState.modeChangedFlag) camState.modeChangedFlag = false;
    }

    view.lookAt = body.position + view.forward;
    body.velocity = (body.position - body.previousPosition) / float(deltaTime);
    body.collisionSphere.currentCenter = body.position;
    if (body.previousPosition != body.position) body.previousPosition = body.position;
    body.deltaMove = glm::vec3(0.0f);

    // Third-person follow: trail behind the body's facing direction, offset upward.
    camState.position = body.position - view.forward * CameraConstants::thirdPersonDistance +
                        CameraConstants::WORLD_UP_AXIS * CameraConstants::thirdPersonHeight;
}

void KinematicController::insertCallbackFunction(int key,
                                                 void (KinematicController::*function)(double)) {
    cameraKeyBindings.emplace(key,
                              [this, function](double deltaTime) { (this->*function)(deltaTime); });
}

void KinematicController::setCameraKeyBindings() {
    insertCallbackFunction(GLFW_KEY_W, &KinematicController::moveForward);
    insertCallbackFunction(GLFW_KEY_A, &KinematicController::moveLeft);
    insertCallbackFunction(GLFW_KEY_S, &KinematicController::moveBackward);
    insertCallbackFunction(GLFW_KEY_D, &KinematicController::moveRight);
}

void KinematicController::onKeyEvent(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        CameraController::toggleFreelook();
    }
}

void KinematicController::onContinuousInput(double deltaTime, const KeyHandling& keyHandler) {
    for (const auto& [key, func] : cameraKeyBindings) {
        if (keyHandler.isKeyPressed(key)) {
            func(deltaTime);
        }
    }
}

double lastX = Config::SCREEN_HEIGHT / 2.0;
double lastY = Config::SCREEN_WIDTH / 2.0;
void KinematicController::mouseCursorEvent(double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse == true) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;  // reversed for typical "screen up is positive pitch"

    lastX = xpos;
    lastY = ypos;

    if (ImGui::GetIO().WantCaptureMouse) return;

    // float newYaw = yaw + glm::radians(xOffset);
    // std::cout << "xOffset: " << xOffset << " radians: " << glm::radians(xOffset)
    //           << " newYaw: " << newYaw << std::endl;

    setYaw(glm::radians(xOffset));
    setPitch(glm::radians(yOffset));
    // update Front, Right and Up Vectors using the updated Euler angles
}

KinematicController::~KinematicController() {
}

KinematicController::KinematicController() {
    // Initial camera location
    firstMouse = true;
    setCameraKeyBindings();
}

glm::vec3 KinematicController::getMovementVector(const glm::vec3& freeVector,
                                                 const glm::vec3& fixedVector) {
    auto& state = CameraController::getCameraState(CameraController::activeCamera);

    if (state.cameraMode == CameraMode::FREE) return freeVector;

    // FIXED mode: flatten and normalize, fall back to last valid direction
    glm::vec3 mv = fixedVector;
    if (glm::length(mv) > 0.001f) {
        mv = glm::normalize(mv);
        lastValidMovementDirection = mv;
    } else {
        mv = lastValidMovementDirection;
    }
    return mv;
}

void KinematicController::moveForward(double deltaTime) {
    auto& body = getActiveKinematicBody();
    glm::vec3 forward =
        glm::normalize(glm::rotate(body.orientation, CameraConstants::WORLD_FORWARD_AXIS));
    body.deltaMove += getMovementVector(forward, glm::vec3(forward.x, 0.0f, forward.z)) *
                      body.speed * (float)deltaTime;
}

void KinematicController::moveBackward(double deltaTime) {
    auto& body = getActiveKinematicBody();
    glm::vec3 forward =
        glm::normalize(glm::rotate(body.orientation, CameraConstants::WORLD_FORWARD_AXIS));
    body.deltaMove -= getMovementVector(forward, glm::vec3(forward.x, 0.0f, forward.z)) *
                      body.speed * (float)deltaTime;
}

void KinematicController::moveRight(double deltaTime) {
    auto& body = getActiveKinematicBody();
    glm::vec3 forward =
        glm::normalize(glm::rotate(body.orientation, CameraConstants::WORLD_FORWARD_AXIS));
    glm::vec3 right =
        glm::normalize(glm::rotate(body.orientation, CameraConstants::WORLD_RIGHT_AXIS));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));
    body.deltaMove += getMovementVector(glm::cross(forward, up),
                                        glm::cross(forward, glm::vec3(up.x, 0.0f, up.z))) *
                      body.speed * (float)deltaTime;
}

void KinematicController::moveLeft(double deltaTime) {
    auto& body = getActiveKinematicBody();
    glm::vec3 forward =
        glm::normalize(glm::rotate(body.orientation, CameraConstants::WORLD_FORWARD_AXIS));
    glm::vec3 right =
        glm::normalize(glm::rotate(body.orientation, CameraConstants::WORLD_RIGHT_AXIS));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));
    body.deltaMove -= getMovementVector(glm::cross(forward, up),
                                        glm::cross(forward, glm::vec3(up.x, 0.0f, up.z))) *
                      body.speed * (float)deltaTime;
}

float wrapAngle(float angle) {
    const float TWO_PI = glm::two_pi<float>();
    angle = fmod(angle, TWO_PI);
    if (angle < 0.0f) angle += TWO_PI;
    return angle;
}

void KinematicController::setPitch(float radians) {
    auto& view = CameraController::getViewState(CameraController::activeCamera);
    float delta = radians * CameraConstants::mouseSensitivity;
    delta = glm::clamp(delta, -glm::radians(CameraConstants::maxPitchRate),
                       glm::radians(CameraConstants::maxPitchRate));
    view.pitch += delta;
    view.pitch = glm::clamp(view.pitch, glm::radians(-89.0f), glm::radians(89.0f));
}

void KinematicController::setYaw(float radians) {
    auto& view = CameraController::getViewState(CameraController::activeCamera);
    float delta = -radians * CameraConstants::mouseSensitivity;
    delta = glm::clamp(delta, -CameraConstants::maxYawRate, CameraConstants::maxYawRate);
    view.yaw += delta;
    view.yaw = wrapAngle(view.yaw);
}

KinematicBody& KinematicController::getActiveKinematicBody() {
    return Registry.get<KinematicBody>(activeKinematicBody);
}