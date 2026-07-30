#pragma once
#include "collision/CollisionResult.hpp"
#include "components/collision/ShapeData.hpp"
#include "components/physics/Orientation.hpp"
#include "components/physics/Position.hpp"
#include "core/PrecompiledHeader.hpp"

class ContactManifold;
class ContactResolver;
class Model;

using EdgeSet = std::pair<std::pair<int, int>, std::array<std::pair<int, int>, 2>>;

class OBB {
   public:
    // Geometry setup
    static void defineBoundingVolume(OBBData& obbData, const GeometricData& geoData);
    static void update(OBBData& obbData, const Position& position, const Orientation& orientation);

    // Collisions
    static CollisionResult collideWithPlane(const OBBData& obb, const PlaneData& plane);
    //  static CollisionResult collideWithAABB(const OBBData& obb, const AABBData& aabb);
    static CollisionResult collideWithTriangle(const OBBData& obb, const TriangleData& triangle);
    static CollisionResult intersectRay(const OBBData& obb, const RayData& ray);
    static CollisionResult collideWithOBB(const OBBData& obb, const OBBData& other);

    // Contact manifolds
    static void populateContactManifoldWithOBB(const OBBData& obb, const OBBData& other,
                                               ContactManifold& manifold,
                                               const CollisionResult& result) {}

    static void populateContactManifoldWithPlane(const OBBData& obb, const PlaneData& plane,
                                                 ContactManifold& manifold,
                                                 const CollisionResult& result);

    static void populateContactManifoldWithConvexHull(const ShapeData& obb,
                                                      const ShapeData& convexHull,
                                                      ContactManifold& manifold,
                                                      const CollisionResult& result);

    static void populateContactManifoldWithTriangle(const OBBData& obb,
                                                    const TriangleData& triangle,
                                                    ContactManifold& manifold,
                                                    const CollisionResult& result);

    static void populateContactManifoldWithSphere(const OBBData& obb, const SphereData& sphere,
                                                  ContactManifold& manifold,
                                                  const CollisionResult& result);

    // Helpers operating on data
    static glm::vec3 calculateContactNormal(const OBBData& obb, const glm::vec3& intersectionPoint);

    // Half-width of the box as measured along an arbitrary direction, i.e. the distance from
    // currentCenter to the box's surface along that direction (accounts for orientation).
    static float getExtentAlongDirection(const OBBData& obb, const glm::vec3& direction);

    static std::vector<int> getFurthestVertices(const OBBData& obb, const glm::vec3& point);

    static glm::vec3 getVertexAtIndex(const OBBData& obb, int index);

    static glm::vec3 getOppositeEdgeCenter(const OBBData& obb,
                                           const std::pair<glm::vec3, glm::vec3>& edge,
                                           const glm::vec3& normal);
};