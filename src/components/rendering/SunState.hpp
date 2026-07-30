#pragma once

struct SunState {
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f};
    glm::vec3 sunColor{0.0f};
    glm::vec3 topColor{0.0f};
    glm::vec3 horizonColor{0.0f};
    glm::vec3 bottomColor{0.0f};
    glm::vec3 skyColorAverage{0.0f};
    float intensity = 10.f;
    float ambientStrength = 0.0f;
    float time = 0.0f;
    float elevation = 0.0f;
};