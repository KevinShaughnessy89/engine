#pragma once
#include "core/PrecompiledHeader.hpp"

struct Material {
    int shadingModel = 0;
    glm::vec3 diffuse = glm::vec3(0.8f);
    glm::vec3 emissive = glm::vec3(0.0f);
    glm::vec3 ambient = glm::vec3(0.0f);
    glm::vec3 specular = glm::vec3(0.8f);
    float shininessPercent = 0.25f;
    float shininess = 36.0f;
    float roughness = 0.4f;
    glm::vec3 transparent = glm::vec3(0.0f);
    float transparencyFactor = 0.0f;
    float opacity = 1.0f;
    glm::vec3 reflective = glm::vec3(0.8f);
    float reflectivity = 0.0f;
    float bumpScaling = 0.0f;
    float displacementScaling = 1.0f;
};
