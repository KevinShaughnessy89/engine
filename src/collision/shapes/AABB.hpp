#pragma once
#include "collision/CollisionAliases.hpp"
#include "collision/CollisionResult.hpp"
#include "collision/shapes/contact/AABBContactDetails.hpp"
#include "collision/shapes/geometry/AABBGeometry.hpp"
#include "components/collision/ShapeData.hpp"
#include "components/physics/Orientation.hpp"
#include "components/physics/Position.hpp"
#include "core/PrecompiledHeader.hpp"

class ContactManifold;
class ContactResolver;

class AABB {
   public:
    // Geometry setup
    static void defineBoundingVolume(AABBData& aabbData, const GeometricData& geoData);
    static void update(AABBData& aabbData, const Position& position,
                       const Orientation& orientation);

    // Collisions
    static CollisionResult collideWithPlane(ShapeData& aabb, const PlaneData& plane);
    static CollisionResult collideWithAABB(const AABBData& aabb, const AABBData& other);
    static CollisionResult intersectRay(ShapeData& aabb, const RayData& ray);

    // Contact manifolds

    // Helpers operating on data
    static glm::vec3 calculateContactNormal(const AABBData& aabb,
                                            const glm::vec3& intersectionPoint);

    static std::vector<int> getFurthestVertices(const AABBData& aabb, const glm::vec3& point);

    static glm::vec3 getVertexAtIndex(const AABBData& aabb, int index);

    static glm::vec3 getOppositeEdgeCenter(const AABBData& aabb,
                                           const std::pair<glm::vec3, glm::vec3>& edge,
                                           const glm::vec3& normal);
};