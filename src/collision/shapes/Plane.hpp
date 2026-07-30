#pragma once
#include "core/PrecompiledHeader.hpp"

#include "collision/shapes/contact/AABBContactDetails.hpp"
#include "collision/shapes/geometry/PlaneGeometry.hpp"
#include "collision/CollisionAliases.hpp"
#include "components/collision/ShapeData.hpp"

class ContactManifold;
class ContactResolver;
class Model;

class Plane {
   public:
    // Geometry setup
    static void defineBoundingVolume(PlaneData& planeData, const GeometricData& geoData);
    static void update(PlaneData& planeData, Model* model);

    // Collisions
    static CollisionResult collideWithPlane(PlaneData& plane, const PlaneData& other);
    static CollisionResult collideWithAABB(PlaneData& plane, const AABBData& aabb);
    static CollisionResult collideWithTriangle(PlaneData& plane, const TriangleData& triangle);
    static CollisionResult intersectRay(PlaneData& plane, const RayData& ray);
    static CollisionResult collideWithOBB(PlaneData& plane, const OBBData& obb);
    static CollisionResult collideWithSphere(PlaneData& plane, const SphereData& sphere);
    static CollisionResult collideWithConvexHull(PlaneData& plane, const ConvexHullData& hull);

    // Contact manifolds
    static void populateContactManifold(PlaneData& plane, const PlaneData& other,
                                        ContactManifold& manifold, const CollisionResult& result);

    static void populateContactManifoldWithOBB(PlaneData& plane, const OBBData& obb,
                                               ContactManifold& manifold,
                                               const CollisionResult& result);

    static void populateContactManifoldWithAABB(PlaneData& plane, const AABBData& aabb,
                                                ContactManifold& manifold,
                                                const CollisionResult& result);

    static void populateContactManifoldWithPlane(PlaneData& plane, const PlaneData& other,
                                                 ContactManifold& manifold,
                                                 const CollisionResult& result);

    static void populateContactManifoldWithConvexHull(PlaneData& plane,
                                                      const ConvexHullData& convexHull,
                                                      ContactManifold& manifold,
                                                      const CollisionResult& result);

    static void populateContactManifoldWithTriangle(PlaneData& plane,
                                                    const TriangleData& triangle,
                                                    ContactManifold& manifold,
                                                    const CollisionResult& result);

    static void populateContactManifoldWithSphere(PlaneData& plane, const SphereData& sphere,
                                                  ContactManifold& manifold,
                                                  const CollisionResult& result);

    // Helpers operating on data
    static glm::vec3 calculateContactNormal(const PlaneData& plane,
                                            const glm::vec3& intersectionPoint);

    static std::vector<int> getFurthestVertices(const PlaneData& plane,
                                                const glm::vec3& point);

    static glm::vec3 getVertexAtIndex(const PlaneData& plane, int index);

    static glm::vec3 getOppositeEdgeCenter(const PlaneData& plane,
                                           const std::pair<glm::vec3, glm::vec3>& edge,
                                           const glm::vec3& normal);
};