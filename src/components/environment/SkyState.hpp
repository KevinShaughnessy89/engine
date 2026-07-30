#pragma once

struct SkyState {
    float time = 0.0f;
    glm::vec3 sunDirection{0.0f};
    glm::vec3 colour{0.0f};
    float size = 0.0f;
    float intensity = 0.0f;

    // Sky colors
    glm::vec3 top{0.0f};
    glm::vec3 horizon{0.0f};
    glm::vec3 bottom{0.0f};
};