#pragma once

#include "CollisionResult.hpp"
#include "collision/shapes/Ray.hpp"
#include "components/collision/ShapeData.hpp"

struct KinematicVisitor {
    RayData& rayData;

    CollisionResult operator()(const SphereData& sphereData) const {
        return CollisionResult{};  // No collision logic for Ray vs Sphere
    }

    CollisionResult operator()(const TriangleData& triangleData) const {
        return Ray::intersectTriangle(rayData, triangleData);
    }

    CollisionResult operator()(const OBBData& obbData) const {
        return CollisionResult{};  // No collision logic for Ray vs OBB
    }
};