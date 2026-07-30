#pragma once

#include "collision/CollisionResult.hpp"
#include "components/collision/ShapeData.hpp"
#include "core/PrecompiledHeader.hpp"

class OBB;
class CollisionShape;
struct CollisionResult;
class PhysicsObject;

class Ray {
   public:
    Ray() = default;
    CollisionResult intersectAABB(const glm::vec3& min, const glm::vec3& max) const;

    static CollisionResult intersectTriangle(const RayData& ray, const TriangleData& triangle);
    static CollisionResult intersectTriangleStraightDown(const RayData& ray,
                                                         const TriangleData& tri);
};