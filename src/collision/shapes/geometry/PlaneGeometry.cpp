#include "PlaneGeometry.hpp"

#include "collision/shapes/Plane.hpp"
#include "core/PrecompiledHeader.hpp"

bool PlaneGeometry::isPointOnPlane(PlaneData& plane, const glm::vec3& point) {
    return (glm::dot(plane.normal, point) < plane.offset + Config::EPSILON &&
            glm::dot(plane.normal, point) > plane.offset - Config::EPSILON);
}

glm::vec3 PlaneGeometry::projectPointOntoPlane(const glm::vec3& point, const glm::vec3& planeNormal,
                                               float planeOffset) {
    float distance = glm::dot(planeNormal, point) - planeOffset;
    return point - distance * planeNormal;
}
