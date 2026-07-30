#pragma once

#include "core/PrecompiledHeader.hpp"

struct Force {
    glm::vec3 accumulated{0.0f};
    glm::vec3 worldTorqueAccumulated{0.0f};
    float magnitude = 0.0f;

    Force() = default;
    Force(float mag) : magnitude(mag) {}
};