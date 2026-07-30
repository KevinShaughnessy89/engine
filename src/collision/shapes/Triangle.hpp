#pragma once

#include <ostream>

#include "collision/CollisionAliases.hpp"
#include "collision/ContactResolver.hpp"
#include "collision/shapes/contact/TriangleContactDetails.hpp"
#include "collision/shapes/geometry/TriangleGeometry.hpp"
#include "components/collision/ShapeData.hpp"
#include "core/PrecompiledHeader.hpp"

class ContactResolver;
class Model;
class ContactManifold;

class Triangle {
   public:
    // Geometry setup
    static void defineBoundingVolume(TriangleData& triangleData);
    static void update(TriangleData& triangleData, const Model* model);

    // Collisions
    static CollisionResult intersectRay(const TriangleData& triangle, const RayData& ray);
    static CollisionResult intersectRayStraightDown(const TriangleData& triangle,
                                                    const RayData& ray);
    static CollisionResult collideWithSphere(const TriangleData& triangle,
                                             const SphereData& sphere);
    static CollisionResult collideWithTriangle(const TriangleData& triangle,
                                               const TriangleData& other) {
        return CollisionResult{};
    }

    // Contact manifolds
    static void populateContactManifoldWithSphere(const TriangleData& triangle,
                                                  const SphereData& sphere,
                                                  ContactManifold& manifold,
                                                  const CollisionResult& result);

    static void populateContactManifoldWithTriangle(const TriangleData& triangle,
                                                    const TriangleData& other,
                                                    ContactManifold& manifold,
                                                    const CollisionResult& result) {}

    // Helpers operating on data
    static glm::vec3 calculateContactNormal(const TriangleData& triangle,
                                            const glm::vec3& intersectionPoint);

    // Triangles are only ever the static side of a collision (terrain), never the dynamic
    // shape being measured, so they have no meaningful extent along a direction.
    static float getExtentAlongDirection(const TriangleData& triangle, const glm::vec3& direction) {
        return 0.0f;
    }

    static std::vector<int> getFurthestVertices(const TriangleData& triangle,
                                                const glm::vec3& point);

    static glm::vec3 getVertexAtIndex(const TriangleData& triangle, int index);

    static glm::vec3 getOppositeEdgeCenter(const TriangleData& triangle,
                                           const std::pair<glm::vec3, glm::vec3>& edge,
                                           const glm::vec3& normal);

    friend std::ostream& operator<<(std::ostream& os, const TriangleData& triangle);
};