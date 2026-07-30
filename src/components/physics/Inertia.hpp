#pragma once

#include "core/PrecompiledHeader.hpp"

struct Inertia {
    glm::mat3 inertiaTensorLocal{1.0f};
    glm::mat3 inverseInertiaTensorLocal{1.0f};
    glm::mat3 inverseInertiaTensorWorld{1.0f};
    glm::mat4 rotationMatrix{1.0f};
};