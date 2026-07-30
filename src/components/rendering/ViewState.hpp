#pragma once

#include "core/PrecompiledHeader.hpp"

// Purely the data needed to produce a view matrix / facing direction -- no position, no
// movement, no collision. Anything that needs "which way is this looking" (the camera today,
// potentially a character later) attaches this alongside its own position source.
struct ViewState {
    glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;

    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 right{0.0f};
    glm::vec3 up{0.0f};
    glm::vec3 lookAt{0.0f};
};
