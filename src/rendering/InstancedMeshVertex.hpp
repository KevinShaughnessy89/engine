#pragma once

struct InstancedMeshVertex {
    glm::vec3 position;
    glm::vec3 size;
    glm::vec3 color;
    glm::vec2 skew;  // x/y skew factor
    float rotation;  // rotation around camera forward
};