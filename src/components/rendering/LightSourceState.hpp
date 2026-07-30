#pragma once

#include "rendering/LightType.hpp"

struct LightSourceState {
    LightType type = LightType::POINT;
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f};
    float cutOff = 0.0f;
    float outerCutOff = 0.0f;
    // Attenuation is 1/(constant + linear*d + quadratic*d^2); constant = 1
    // keeps an unconfigured light finite instead of dividing by zero.
    float constant = 1.0f;
    float linear = 0.0f;
    float quadratic = 0.0f;
    glm::vec3 ambient{0.0f};
    glm::vec3 diffuse{0.0f};
    glm::vec3 specular{0.0f};
};