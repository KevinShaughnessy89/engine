#pragma once

struct Torque {
    glm::vec3 worldTorqueAccumulated{0.0f};
    glm::vec3 localTorqueAccumulated{0.0f};
};