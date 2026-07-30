#pragma once

#include "ContactPoint.hpp"

struct CollisionResult {
    ContactPoint contactPoint;
    bool collided = false;
    operator bool() const { return collided; }
    float t = 0.f;

    CollisionResult() = default;
    CollisionResult(bool collided, const glm::vec3& intersectionPoint, const glm::vec3& normal,
                    float penetrationDepth)
        : collided(collided), contactPoint(intersectionPoint, normal, penetrationDepth) {}
    CollisionResult(const CollisionResult& other)
        : collided(other.collided), contactPoint(other.contactPoint) {}
};
