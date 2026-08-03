#pragma once

#include "components/collision/ShapeData.hpp"

enum class MovementState { RUN, WALK };
enum class MovementDirection { NONE, FORWARD, BACKWARD };

// Movement/collision state for anything driven directly by input rather than force integration
// (the player character's body) -- position, grounding, and the collider used to resolve against
// the world via KinematicResolver.
struct KinematicBody {
    glm::vec3 position{0.0f};
    glm::vec3 previousPosition{0.0f};
    glm::vec3 targetPosition{0.0f};  // desired position this step, before collision resolution
    glm::vec3 velocity{0.0f};
    glm::vec3 acceleration{0.0f};
    glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;  // facing, independent of ViewState.yaw -- a body only turns, it doesn't
                       // pitch; written by MovementController::setYaw in FIXED mode only
    glm::vec3 deltaPos{0.0f};     // per-frame requested movement; written by MovementController,
                                  // consumed and reset by KinematicController::update
    MovementDirection movementDirection = MovementDirection::NONE;  // set by MovementController's
                                  // move* functions, selects Walk/Run clip direction; consumed
                                  // and reset by CharacterController::update
    MovementState movementState = MovementState::RUN;

    float speed = 4000.0f;
    float verticalVelocity = 0.0f;
    bool isGrounded = false;
    bool wasGrounded = false;
    float groundClearance = 0.0f;  // minimum clearance needed to clear steep slopes without
                                   // losing grounding

    SphereData collisionSphere;

    KinematicBody() {
        collisionSphere.radius = 0.5f;
        collisionSphere.currentCenter = position;
        collisionSphere.initialCenter = position;
        collisionSphere.previousCenter = position;
    }
};
