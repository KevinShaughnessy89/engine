#include "collision/shapes/AABB.hpp"

#include "collision/ContactResolver.hpp"
#include "collision/shapes/ConvexHull.hpp"
#include "collision/shapes/OBB.hpp"
#include "collision/shapes/Plane.hpp"
#include "collision/shapes/Ray.hpp"
#include "collision/shapes/Sphere.hpp"
#include "collision/shapes/Triangle.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/Model.hpp"

void AABB::defineBoundingVolume(AABBData& aabb, const GeometricData& geoData) {
    aabb.max = geoData.max;
    aabb.min = geoData.min;

    float rangeX = aabb.max.x - aabb.min.x;
    float rangeY = aabb.max.y - aabb.min.y;
    float rangeZ = aabb.max.z - aabb.min.z;

    glm::vec3 initialCenter =
        glm::vec3((aabb.max.x + aabb.min.x) / 2.0f, (aabb.max.y + aabb.min.y) / 2.0f,
                  (aabb.max.z + aabb.min.z) / 2.0f);

    aabb.halfExtents = glm::vec3(rangeX / 2.0f, rangeY / 2.0f, rangeZ / 2.0f);

    aabb.currentCenter = initialCenter;
    aabb.initialCenter = initialCenter;
}

void AABB::update(AABBData& aabb, const Position& position, const Orientation& orientation) {
    aabb.currentCenter = position.current + orientation.rotationMatrix * aabb.initialCenter;

    aabb.min = aabb.currentCenter - aabb.halfExtents;
    aabb.max = aabb.currentCenter + aabb.halfExtents;
}

CollisionResult AABB::collideWithAABB(const AABBData& aabb, const AABBData& other) {
    CollisionResult result;

    glm::vec3 distance = glm::abs(aabb.currentCenter - other.currentCenter);
    glm::vec3 sumHalfExtents = aabb.halfExtents + other.halfExtents;

    if (glm::all(glm::lessThanEqual(distance, sumHalfExtents))) {
        result.collided = true;
    }

    return result;
}

glm::vec3 AABB::calculateContactNormal(const AABBData& aabb, const glm::vec3& intersectionPoint) {
    return glm::vec3(0.0f);
}

std::vector<int> AABB::getFurthestVertices(const AABBData& aabb, const glm::vec3& point) {
    return std::vector<int>();
}

glm::vec3 AABB::getVertexAtIndex(const AABBData& aabb, int index) {
    return glm::vec3(0.0f);
}

glm::vec3 AABB::getOppositeEdgeCenter(const AABBData& aabb,
                                      const std::pair<glm::vec3, glm::vec3>& edge,
                                      const glm::vec3& normal) {
    return glm::vec3(0.0f);
}