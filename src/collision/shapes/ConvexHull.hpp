// #pragma once

// #include "collision/CollisionAliases.hpp"
// #include "collision/CollisionResult.hpp"
// #include "components/collision/ShapeData.hpp"
// #include "core/PrecompiledHeader.hpp"

// class ContactManifold;
// class ContactResolver;
// class Model;

// class ConvexHull {
//    public:
//     ConvexHull();
//     // Geometry setup
//     static void defineBoundingVolume(const ConvexHullData& hull, const GeometricData& geoData);
//     static void update(ConvexHullData& hull, Model* model);

//     // Collisions
//     static CollisionResult collideWithPlane(ConvexHullData& hull, const PlaneData& plane);
//     static CollisionResult collideWithAABB(ConvexHullData& hull, const AABBData& aabb);
//     static CollisionResult collideWithTriangle(ConvexHullData& hull, const TriangleData&
//     triangle); static CollisionResult intersectRay(ConvexHullData& hull, const RayData& ray);
//     static CollisionResult collideWithOBB(ConvexHullData& hull, const OBBData& obb);
//     static CollisionResult collideWithSphere(ConvexHullData& hull, const SphereData& sphere);
//     static CollisionResult collideWithConvexHull(ConvexHullData& hull, const ConvexHullData&
//     other);

//     // Contact manifolds
//     static void populateContactManifold(ConvexHullData& hull, const PlaneData& plane,
//                                         ContactManifold& manifold, const CollisionResult& result,
//                                         ContactResolver& contactResolver);

//     static void populateContactManifoldWithOBB(ConvexHullData& hull, const OBBData& obb,
//                                                ContactManifold& manifold,
//                                                const CollisionResult& result,
//                                                ContactResolver& contactResolver);

//     static void populateContactManifoldWithAABB(ConvexHullData& hull, const AABBData& aabb,
//                                                 ContactManifold& manifold,
//                                                 const CollisionResult& result,
//                                                 ContactResolver& contactResolver);

//     static void populateContactManifoldWithPlane(ConvexHullData& hull, const PlaneData& plane,
//                                                  ContactManifold& manifold,
//                                                  const CollisionResult& result,
//                                                  ContactResolver& contactResolver);

//     static void populateContactManifoldWithConvexHull(ConvexHullData& hull,
//                                                       const ConvexHullData& other,
//                                                       ContactManifold& manifold,
//                                                       const CollisionResult& result,
//                                                       ContactResolver& contactResolver);

//     static void populateContactManifoldWithTriangle(ConvexHullData& hull,
//                                                     const TriangleData& triangle,
//                                                     ContactManifold& manifold,
//                                                     const CollisionResult& result,
//                                                     ContactResolver& contactResolver);

//     static void populateContactManifoldWithSphere(ConvexHullData& hull, const SphereData& sphere,
//                                                   ContactManifold& manifold,
//                                                   const CollisionResult& result,
//                                                   ContactResolver& contactResolver);

//     // Helpers operating on data
//     static glm::vec3 calculateContactNormal(const ConvexHullData& hull,
//                                             const glm::vec3& intersectionPoint);

//     static std::vector<int> getFurthestVertices(const ConvexHullData& hull, const glm::vec3&
//     point);

//     static glm::vec3 getVertexAtIndex(const ConvexHullData& hull, int index);

//     static glm::vec3 getOppositeEdgeCenter(const ConvexHullData& hull,
//                                            const std::pair<glm::vec3, glm::vec3>& edge,
//                                            const glm::vec3& normal);
// };