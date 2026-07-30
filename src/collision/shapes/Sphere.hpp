#pragma once

#include "collision/CollisionResult.hpp"
#include "components/collision/ShapeData.hpp"
#include "components/physics/Orientation.hpp"
#include "components/physics/Position.hpp"
#include "core/PrecompiledHeader.hpp"

class Model;
class ContactResolver;
class ContactManifold;

class Sphere {
   public:
    Sphere() = default;

    // Geometry setup
    static void defineRadialLength(SphereData& sphereData, const GeometricData& geoData);
    static void defineCenterPosition(SphereData& sphereData, const glm::vec3& position);
    static void update(SphereData& sphereData, const Position& position,
                       const Orientation& orientation);

    // Collisions
    static CollisionResult collideWithAABB(const SphereData& sphere, const AABBData& aabb);
    static CollisionResult collideWithTriangle(const SphereData& sphere,
                                               const TriangleData& triangle);
    static CollisionResult collideWithOBB(const SphereData& sphere, const OBBData& obb);
    static CollisionResult collideWithSphere(const SphereData& sphere, const SphereData& other);
    static CollisionResult collideWithConvexHull(const ShapeData& sphere, const ShapeData& hull);

    static CollisionResult sphereSweepAgainstTriangle(const SphereData& sphere,
                                                      const TriangleData& triangle);

    static void populateContactManifoldWithOBB(const SphereData& sphere, const OBBData& obb,
                                               ContactManifold& manifold,
                                               const CollisionResult& result);

    static void populateContactManifoldWithPlane(const SphereData& sphere, const PlaneData& plane,
                                                 ContactManifold& manifold,
                                                 const CollisionResult& result);

    static void populateContactManifoldWithTriangle(const SphereData& sphere,
                                                    const TriangleData& triangle,
                                                    ContactManifold& manifold,
                                                    const CollisionResult& result);

    static void populateContactManifoldWithSphere(const SphereData& sphere, const SphereData& other,
                                                  ContactManifold& manifold,
                                                  const CollisionResult& result);

    // Helpers operating on data
    static glm::vec3 calculateContactNormal(const SphereData& sphere,
                                            const glm::vec3& intersectionPoint) {
        return glm::vec3(0.f);
    }

    // Half-width of the shape as measured along an arbitrary direction, i.e. the distance
    // from currentCenter to the shape's surface along that direction. A sphere's radius is
    // the same in every direction.
    static float getExtentAlongDirection(const SphereData& sphere, const glm::vec3& direction) {
        return sphere.radius;
    }

    static std::vector<int> getFurthestVertices(const SphereData& sphere, const glm::vec3& point);

    static glm::vec3 getVertexAtIndex(const SphereData& sphere, int index);

    static glm::vec3 getOppositeEdgeCenter(const SphereData& sphere,
                                           const std::pair<glm::vec3, glm::vec3>& edge,
                                           const glm::vec3& normal);
};