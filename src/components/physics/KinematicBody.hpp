#pragma once

#include "components/collision/ShapeData.hpp"

// Movement/collision state for anything driven directly by input rather than force integration
// (the camera in FIXED mode today, a player character later) -- position, grounding, and the
// collider used to resolve against the world via KinematicResolver.
struct KinematicBody {
    glm::vec3 position{0.0f};
    glm::vec3 previousPosition{0.0f};
    glm::vec3 targetPosition{0.0f};  // desired position this step, before collision resolution
    glm::vec3 velocity{0.0f};
    glm::vec3 acceleration{0.0f};
    glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 deltaMove{0.0f};  // accumulated input displacement; consumed and reset each update

    float speed = 4000.f;
    float verticalVelocity = 0.0f;
    bool isGrounded = false;
    bool wasGrounded = false;
    float groundClearance = 20.75f;  // minimum clearance needed to clear steep slopes without
                                     // losing grounding

    SphereData collisionSphere;

    KinematicBody() {
        collisionSphere.radius = 0.5f;
        collisionSphere.currentCenter = position;
        collisionSphere.initialCenter = position;
        collisionSphere.previousCenter = position;
    }
};
