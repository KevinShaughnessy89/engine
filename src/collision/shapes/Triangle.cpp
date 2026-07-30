#include "collision/shapes/Triangle.hpp"

#include "collision/ContactManifold.hpp"
#include "collision/ContactResolver.hpp"
#include "collision/shapes/AABB.hpp"
#include "collision/shapes/ConvexHull.hpp"
#include "collision/shapes/OBB.hpp"
#include "collision/shapes/Plane.hpp"
#include "collision/shapes/Ray.hpp"
#include "collision/shapes/Sphere.hpp"
#include "collision/shapes/contact/TriangleContactDetails.hpp"
#include "collision/shapes/geometry/TriangleGeometry.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/ShaderProgram.hpp"


void Triangle::defineBoundingVolume(TriangleData& triangle) {
    TriangleGeometry::calculateTriangleMinMax(triangle);

    glm::vec3 edge1 = triangle.vertices[1] - triangle.vertices[0];
    glm::vec3 edge2 = triangle.vertices[2] - triangle.vertices[0];
    triangle.normal = glm::normalize(glm::cross(edge1, edge2));
}

void Triangle::update(TriangleData& triangleData, const Model* model) {
}

CollisionResult Triangle::intersectRay(const TriangleData& tri, const RayData& r) {
    const float EPSILON = 1e-6f;
    const float BARY_EPS = 1e-4f;

    CollisionResult result;

    glm::vec3 v1 = tri.vertices[0];
    glm::vec3 v2 = tri.vertices[1];
    glm::vec3 v3 = tri.vertices[2];

    glm::vec3 rayDir = glm::normalize(r.direction);
    glm::vec3 edge1 = v2 - v1;
    glm::vec3 edge2 = v3 - v1;

    glm::vec3 s = glm::cross(rayDir, edge2);
    float det = glm::dot(edge1, s);

    if (std::abs(det) < EPSILON) {
        return result;  // Parallel ray or degenerate triangle
    }

    float invDet = 1.0f / det;
    glm::vec3 rayToVertex = r.origin - v1;
    float u = invDet * glm::dot(rayToVertex, s);

    if (u < -BARY_EPS || u > 1.0f + BARY_EPS) {
        return result;
    }

    glm::vec3 q = glm::cross(rayToVertex, edge1);
    float v = invDet * glm::dot(rayDir, q);

    if (v < -BARY_EPS || (u + v) > 1.0f + BARY_EPS) {
        return result;
    }

    float t = invDet * glm::dot(edge2, q);

    if (t > EPSILON) {
        result.collided = true;
        result.contactPoint.position = r.origin + rayDir * t;
        result.t = t;
    }

    return result;
}

CollisionResult Triangle::intersectRayStraightDown(const TriangleData& tri, const RayData& r) {
    const float EPSILON = 0.0000001f;

    CollisionResult result;

    // Check if ray origin is very close to any vertex
    for (int i = 0; i < 3; ++i) {
        if (glm::length(r.origin - tri.vertices[i]) < EPSILON) {
            result.collided = true;
            result.contactPoint.position = tri.vertices[i];
            return result;
        }
    }

    return intersectRay(tri, r);
}

CollisionResult Triangle::collideWithSphere(const TriangleData& tri, const SphereData& s) {
    CollisionResult result;

    // Compute the closest point on the triangle to the sphere center
    glm::vec3 closestPoint = TriangleGeometry::closestPointOnTriangle(tri, s.currentCenter);

    // Compute the vector from closest point to the sphere center
    glm::vec3 diff = s.currentCenter - closestPoint;
    float distanceSquared = glm::dot(diff, diff);

    // Check for collision
    if (distanceSquared <= s.radius * s.radius) {
        result.collided = true;
        result.contactPoint.position = closestPoint;

        float distance = std::sqrt(distanceSquared);
        result.contactPoint.penetrationDepth = s.radius - distance;

        // Radial normal (closest point -> sphere center) is correct by construction and
        // doesn't depend on mesh winding; only fall back to the triangle's face normal in
        // the degenerate case where the sphere center sits exactly on the surface.
        if (distance > 1e-6f) {
            result.contactPoint.normal = diff / distance;
        } else {
            result.contactPoint.normal = tri.normal;

            glm::vec3 triangleCenter = (tri.vertices[0] + tri.vertices[1] + tri.vertices[2]) / 3.0f;
            glm::vec3 toSphere = s.currentCenter - triangleCenter;

            if (glm::dot(result.contactPoint.normal, toSphere) < 0.0f) {
                result.contactPoint.normal = -result.contactPoint.normal;
            }
        }

        result.t = distance;
    }
    return result;
}

void Triangle::populateContactManifoldWithSphere(const TriangleData& tri, const SphereData& s,
                                                 ContactManifold& manifold,
                                                 const CollisionResult& result) {
    // 1. Closest point on triangle to sphere center
    glm::vec3 closest = TriangleGeometry::closestPointOnTriangle(tri, s.currentCenter);

    // 2. Vector from closest point to sphere center
    glm::vec3 v = s.currentCenter - closest;
    float distSq = glm::dot(v, v);
    float radiusSq = s.radius * s.radius;

    if (distSq <= radiusSq) {
        float dist = std::sqrt(distSq);

        glm::vec3 normal;
        if (dist > 1e-6f) {
            normal = v / dist;  // radial
        } else {
            normal = tri.normal;  // fallback
        }

        if (glm::dot(normal, v) < 0.0f) normal = -normal;

        float penetration = s.radius - dist;
        glm::vec3 contactOnSphere = s.currentCenter - normal * s.radius;

        manifold.addContactPoint(contactOnSphere, normal, penetration);
    }
}