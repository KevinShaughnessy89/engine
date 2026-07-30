#pragma once

#include "ContactPoint.hpp"
#include "core/PrecompiledHeader.hpp"


struct ResolvedConstraint {
    glm::vec3 position;
    bool isGrounded;
};