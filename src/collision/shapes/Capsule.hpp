#pragma once

#include "collision/CollisionResult.hpp"
#include "collision/ContactManifold.hpp"
#include "components/collision/ShapeData.hpp"
#include "geometry/GeometryObject.hpp"

class Capsule {
   public:
    static void defineBoundingVolume(CapsuleData& capsuleData, const GeometricData& geometricData) {
        capsuleData.min = geometricData.min;
        capsuleData.max = geometricData.max;
        capsuleData.currentCenter = (geometricData.max + geometricData.min) * 0.5f;
        capsuleData.initialCenter = (geometricData.max + geometricData.min) * 0.5f;
        capsuleData.radius = (geometricData.max.x - geometricData.min.x) * 0.5f;
        capsuleData.height = geometricData.max.y - geometricData.min.y;
    }

    static CollisionResult intersectWithSphere(const CapsuleData& capsule,
                                               const SphereData& sphere) {
        glm::vec3 segmentA = capsule.currentCenter + capsule.axis * (capsule.height / 2.f);
        glm::vec3 segmentB = capsule.currentCenter - capsule.axis * (capsule.height / 2.f);

        glm::vec3 closest =
            GeometryObject::closestPointOnLine(sphere.currentCenter, segmentA, segmentB);

        glm::vec3 delta = sphere.currentCenter - closest;
        float distanceSq = glm::dot(delta, delta);
        float radiusSum = sphere.radius + capsule.radius;

        if (distanceSq >= radiusSum * radiusSum) {
            return CollisionResult();
        }

        float distance = std::sqrt(distanceSq);

        CollisionResult result;
        // Degenerate case: sphere center lands exactly on the segment (dist == 0).
        // Pick an arbitrary normal perpendicular to the capsule axis to avoid NaN.
        if (distance < 1e-6f) {
            glm::vec3 arbitrary =
                (std::abs(capsule.axis.y) < 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
            result.contactPoint.normal = glm::normalize(glm::cross(capsule.axis, arbitrary));
            distance = 0.0f;
        } else {
            result.contactPoint.normal = delta / distance;
        }

        result.collided = true;
        result.contactPoint.penetrationDepth = radiusSum - distance;
        result.contactPoint.position = closest + result.contactPoint.normal * capsule.radius;

        return result;
    }

    static CollisionResult intersectWithOBB(const CapsuleData& capsule, const OBBData& obb) {
        // Implement capsule-OBB intersection logic here
        // This is a placeholder for the actual implementation
        CollisionResult result;
        // Perform intersection test and populate result accordingly
        return result;
    }

    static void populateContactManifoldWithSphere(const CapsuleData& capsule,
                                                  const SphereData& sphere,
                                                  ContactManifold& manifold,
                                                  const CollisionResult& result) {
        manifold.addContactPoint(result.contactPoint.position, result.contactPoint.normal,
                                 result.contactPoint.penetrationDepth);
    }

    static void populateContactManifoldWithOBB(const CapsuleData& capsule, const OBBData& obb,
                                               ContactManifold& manifold,
                                               const CollisionResult& result) {
        // Implement contact manifold population logic for capsule-OBB collision
        // This is a placeholder for the actual implementation
    }

    static float getExtentAlongDirection(const CapsuleData& capsule, const glm::vec3& direction) {
        // Project the capsule's axis onto the direction vector
        return capsule.radius +
               (capsule.height / 2.0f) * std::abs(glm::dot(capsule.axis, direction));
    }
};