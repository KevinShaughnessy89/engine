#include "GeometryObject.hpp"

#include "core/PrecompiledHeader.hpp"
#include "rendering/Model.hpp"


glm::vec3 GeometryObject::closestPointOnLine(const glm::vec3& point, const glm::vec3& lineStart,
                                             const glm::vec3& lineEnd) {
    glm::vec3 line = lineEnd - lineStart;
    float t = glm::dot(point - lineStart, line) / glm::dot(line, line);
    t = glm::clamp(t, 0.0f, 1.0f);
    return lineStart + t * line;
}
