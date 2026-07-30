#include "AABBGeometry.hpp"

#include "collision/shapes/AABB.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"

std::array<glm::vec3, 8> AABBGeometry::getAABBCorners(const AABBData& aabb) {
    std::array<glm::vec3, 8> corners;

    for (int i = 0; i < 8; ++i) {
        corners[i] =
            glm::vec3(aabb.currentCenter.x + ((i & 4) ? aabb.halfExtents.x : -aabb.halfExtents.x),
                      aabb.currentCenter.y + ((i & 2) ? aabb.halfExtents.y : -aabb.halfExtents.y),
                      aabb.currentCenter.z + ((i & 1) ? aabb.halfExtents.z : -aabb.halfExtents.z));
    }
    return corners;
}

void AABBGeometry::projectAABBOntoAxis(const AABBData& aabb, const glm::vec3& axis, float& min,
                                       float& max) {
    std::array<glm::vec3, 8> cornerVectors = getAABBCorners(aabb);
    for (const auto& corner : cornerVectors) {
        min = std::min(min, glm::dot(corner, axis));
        max = std::max(max, glm::dot(corner, axis));
    }
}

bool AABBGeometry::containsPoint(const AABBData& aabb, const glm::vec3& point) {
    return (point.x >= aabb.min.x && point.x <= aabb.max.x && point.y >= aabb.min.y &&
            point.y <= aabb.max.y && point.z >= aabb.min.z && point.z <= aabb.max.z);
}
