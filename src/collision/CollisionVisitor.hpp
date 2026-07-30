#pragma once

#include "collision/CollisionResult.hpp"
#include "collision/ContactManifold.hpp"
#include "collision/shapes/Capsule.hpp"
#include "collision/shapes/OBB.hpp"
#include "collision/shapes/Sphere.hpp"
#include "collision/shapes/Triangle.hpp"
#include "components/collision/ShapeData.hpp"

using ManifoldCallback = std::function<void(const ShapeData&, const ShapeData&, ContactManifold&,
                                            const CollisionResult&)>;
using DetectionCallback = std::function<CollisionResult(const ShapeData&, const ShapeData&)>;

struct CollisionVisitor {
    CollisionResult operator()(const SphereData& a, const SphereData& b) const {
        return Sphere::collideWithSphere(a, b);
    }
    CollisionResult operator()(const SphereData& a, const TriangleData& b) const {
        return Sphere::collideWithTriangle(a, b);
    }
    CollisionResult operator()(const TriangleData& a, const SphereData& b) const {
        return Sphere::collideWithTriangle(b, a);
    }
    CollisionResult operator()(const SphereData& a, const OBBData& b) const {
        return Sphere::collideWithOBB(a, b);
    }
    CollisionResult operator()(const OBBData& a, const SphereData& b) const {
        return Sphere::collideWithOBB(b, a);
    }
    CollisionResult operator()(const TriangleData& a, const TriangleData& b) const {
        return Triangle::collideWithTriangle(a, b);
    }
    CollisionResult operator()(const TriangleData& a, const OBBData& b) const {
        return OBB::collideWithTriangle(b, a);
    }
    CollisionResult operator()(const OBBData& a, const TriangleData& b) const {
        return OBB::collideWithTriangle(a, b);
    }
    CollisionResult operator()(const OBBData& a, const OBBData& b) const {
        return OBB::collideWithOBB(a, b);
    }
    CollisionResult operator()(const CapsuleData& a, const SphereData& b) const {
        return Capsule::intersectWithSphere(a, b);
    }
    CollisionResult operator()(const SphereData& a, const CapsuleData& b) const {
        return Capsule::intersectWithSphere(b, a);
    }
    CollisionResult operator()(const CapsuleData& a, const OBBData& b) const {
        return Capsule::intersectWithOBB(a, b);
    }
    CollisionResult operator()(const OBBData& a, const CapsuleData& b) const {
        return Capsule::intersectWithOBB(b, a);
    }
    CollisionResult operator()(const CapsuleData& a, const TriangleData& b) const {
        return CollisionResult();  // Placeholder for capsule-triangle intersection
    }
    CollisionResult operator()(const TriangleData& a, const CapsuleData& b) const {
        return CollisionResult();  // Placeholder for triangle-capsule intersection
    }
    CollisionResult operator()(const CapsuleData& a, const CapsuleData& b) const {
        return CollisionResult();  // Placeholder for capsule-capsule intersection
    }
};

struct ManifoldVisitor {
    ContactManifold& manifold;
    const CollisionResult& result;

    void operator()(const SphereData& a, const SphereData& b) const {
        Sphere::populateContactManifoldWithSphere(a, b, manifold, result);
    }
    void operator()(const SphereData& a, const TriangleData& b) const {
        Sphere::populateContactManifoldWithTriangle(a, b, manifold, result);
    }
    void operator()(const TriangleData& a, const SphereData& b) const {
        Sphere::populateContactManifoldWithTriangle(b, a, manifold, result);
    }
    void operator()(const SphereData& a, const OBBData& b) const {
        Sphere::populateContactManifoldWithOBB(a, b, manifold, result);
    }
    void operator()(const OBBData& a, const SphereData& b) const {
        Sphere::populateContactManifoldWithOBB(b, a, manifold, result);
    }
    void operator()(const TriangleData& a, const TriangleData& b) const {
        Triangle::populateContactManifoldWithTriangle(a, b, manifold, result);
    }
    void operator()(const TriangleData& a, const OBBData& b) const {
        OBB::populateContactManifoldWithTriangle(b, a, manifold, result);
    }
    void operator()(const OBBData& a, const TriangleData& b) const {
        OBB::populateContactManifoldWithTriangle(a, b, manifold, result);
    }
    void operator()(const OBBData& a, const OBBData& b) const {
        OBB::populateContactManifoldWithOBB(a, b, manifold, result);
    }
    void operator()(const CapsuleData& a, const SphereData& b) const {
        Capsule::populateContactManifoldWithSphere(a, b, manifold, result);
    }
    void operator()(const SphereData& a, const CapsuleData& b) const {
        Capsule::populateContactManifoldWithSphere(b, a, manifold, result);
    }
    void operator()(const CapsuleData& a, const OBBData& b) const {
        Capsule::populateContactManifoldWithOBB(a, b, manifold, result);
    }
    void operator()(const OBBData& a, const CapsuleData& b) const {
        Capsule::populateContactManifoldWithOBB(b, a, manifold, result);
    }
    void operator()(const CapsuleData& a, const TriangleData& b) const {
        // Placeholder for capsule-triangle contact manifold population
    }
    void operator()(const TriangleData& a, const CapsuleData& b) const {
        // Placeholder for triangle-capsule contact manifold population
    }
    void operator()(const CapsuleData& a, const CapsuleData& b) const {
        // Placeholder for capsule-capsule contact manifold population
    }
};

// Half-width of whichever shape is held, measured along `direction` (see
// Sphere/OBB/Triangle::getExtentAlongDirection).
struct ExtentAlongDirectionVisitor {
    glm::vec3 direction;

    float operator()(const SphereData& s) const {
        return Sphere::getExtentAlongDirection(s, direction);
    }
    float operator()(const CapsuleData& c) const {
        return Capsule::getExtentAlongDirection(c, direction);
    }
    float operator()(const OBBData& o) const { return OBB::getExtentAlongDirection(o, direction); }
    float operator()(const TriangleData& t) const {
        return Triangle::getExtentAlongDirection(t, direction);
    }
};