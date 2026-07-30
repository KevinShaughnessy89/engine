#include "Plane.hpp"

#include "collision/ContactManifold.hpp"
#include "collision/ContactResolver.hpp"
#include "collision/shapes/AABB.hpp"
#include "collision/shapes/ConvexHull.hpp"
#include "collision/shapes/OBB.hpp"
#include "collision/shapes/Ray.hpp"
#include "collision/shapes/Sphere.hpp"
#include "collision/shapes/Triangle.hpp"
#include "collision/shapes/contact/PlaneContactDetails.hpp"
#include "core/PrecompiledHeader.hpp"


void Plane::defineBoundingVolume(PlaneData& plane, const GeometricData& geoData) {
    // Implementation
}

void Plane::update(PlaneData& plane, Model* model) {
    // Implementation
}

CollisionResult Plane::collideWithPlane(PlaneData& plane, const PlaneData& other) {
    CollisionResult result;

    float relativeAngle = glm::dot(plane.normal, plane.normal);

    if (std::abs(relativeAngle) < 1.0f - Config::EPSILON) {
        result.collided = true;
        result.contactPoint.normal = plane.normal;
        result.contactPoint.position =
            PlaneContactDetails::getPlanePlaneIntersectionPoint(plane, other);
    }

    return result;
}

CollisionResult Plane::collideWithAABB(PlaneData& plane, const AABBData& aabb) {
    CollisionResult result;

    float rProj = (std::abs(plane.normal.x) * aabb.halfExtents.x +
                   std::abs(plane.normal.y) * aabb.halfExtents.y +
                   std::abs(plane.normal.z) * aabb.halfExtents.z);

    float distance = glm::dot(plane.normal, aabb.currentCenter) - plane.offset;

    result.collided = std::abs(distance) <= rProj;

    return result;
}

CollisionResult Plane::collideWithTriangle(PlaneData& plane, const TriangleData& triangle) {
    return CollisionResult();
}

CollisionResult Plane::intersectRay(PlaneData& plane, const RayData& ray) {
    return CollisionResult();
}

CollisionResult Plane::collideWithOBB(PlaneData& plane, const OBBData& obb) {
    return CollisionResult();
}

CollisionResult Plane::collideWithSphere(PlaneData& plane, const SphereData& sphere) {
    return CollisionResult();
}

CollisionResult Plane::collideWithConvexHull(PlaneData& plane, const ConvexHullData& hull) {
    return CollisionResult();
}

void Plane::populateContactManifold(PlaneData& plane, const PlaneData& other,
                                    ContactManifold& manifold, const CollisionResult& result) {
    // Implementation
}

void Plane::populateContactManifoldWithOBB(PlaneData& plane, const OBBData& obb,
                                           ContactManifold& manifold,
                                           const CollisionResult& result) {
    int maxContact = 4;

    for (const auto& vertex : obb.currentCorners) {
        if (maxContact > 0) {
            float distance = glm::dot(plane.normal, vertex) - plane.offset;

            if (glm::epsilonEqual(distance, 0.0f, Config::EPSILON)) {
                manifold.addContactPoint(vertex, plane.normal, 0.0f);
                maxContact--;
            } else if (distance < 0) {
                manifold.addContactPoint(vertex, plane.normal, -distance);
                maxContact--;
            }
        }
    }
}

void Plane::populateContactManifoldWithAABB(PlaneData& plane, const AABBData& aabb,
                                            ContactManifold& manifold,
                                            const CollisionResult& result) {
    // Implementation
}

void Plane::populateContactManifoldWithPlane(PlaneData& plane, const PlaneData& other,
                                             ContactManifold& manifold,
                                             const CollisionResult& result) {
    manifold.addContactPoint(
        result.contactPoint.position, result.contactPoint.normal,
        PlaneContactDetails::getPenetrationDepth(other, result.contactPoint.position));
}

void Plane::populateContactManifoldWithConvexHull(PlaneData& plane,
                                                  const ConvexHullData& convexHull,
                                                  ContactManifold& manifold,
                                                  const CollisionResult& result) {
    // Implementation
}

void Plane::populateContactManifoldWithTriangle(PlaneData& plane, const TriangleData& triangle,
                                                ContactManifold& manifold,
                                                const CollisionResult& result) {
    // Implementation
}

void Plane::populateContactManifoldWithSphere(PlaneData& plane, const SphereData& sphere,
                                              ContactManifold& manifold,
                                              const CollisionResult& result) {
    // Implementation
}

glm::vec3 Plane::calculateContactNormal(const PlaneData& plane,
                                        const glm::vec3& intersectionPoint) {
    return glm::vec3(0.0f);
}

std::vector<int> Plane::getFurthestVertices(const PlaneData& plane, const glm::vec3& point) {
    return std::vector<int>();
}

glm::vec3 Plane::getVertexAtIndex(const PlaneData& plane, int index) {
    return glm::vec3(0.0f);
}

glm::vec3 Plane::getOppositeEdgeCenter(const PlaneData& plane,
                                       const std::pair<glm::vec3, glm::vec3>& edge,
                                       const glm::vec3& normal) {
    return glm::vec3(0.0f);
}