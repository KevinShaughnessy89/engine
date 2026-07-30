#include "Sphere.hpp"

#include "collision/ContactManifold.hpp"
#include "collision/ContactResolver.hpp"
#include "collision/shapes/AABB.hpp"
#include "collision/shapes/ConvexHull.hpp"
#include "collision/shapes/OBB.hpp"
#include "collision/shapes/Plane.hpp"
#include "collision/shapes/Ray.hpp"
#include "collision/shapes/Triangle.hpp"
#include "collision/shapes/contact/SphereContactDetails.hpp"
#include "collision/shapes/geometry/GeometryObject.hpp"
#include "collision/shapes/geometry/PlaneGeometry.hpp"
#include "collision/shapes/geometry/SphereGeometry.hpp"
#include "collision/shapes/geometry/TriangleGeometry.hpp"
#include "components/collision/ShapeData.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/Model.hpp"
#include "rendering/ShaderProgram.hpp"

void Sphere::defineRadialLength(SphereData& sphereData, const GeometricData& geoData) {
    sphereData.max = geoData.max;
    sphereData.min = geoData.min;

    sphereData.radius = (sphereData.max.x - sphereData.min.x) / 2;
    sphereData.radiusDefined = true;
}

void Sphere::defineCenterPosition(SphereData& sphereData, const glm::vec3& position) {
    // initialCenter is local-space (mesh-local offset)
    // Sphere::update() adds position.current to it every tick.
    sphereData.initialCenter = (sphereData.min + sphereData.max) / 2.0f;
    sphereData.currentCenter = position + sphereData.initialCenter;
    sphereData.centerDefined = true;
}

void Sphere::update(SphereData& sphereData, const Position& position,
                    const Orientation& orientation) {
    if (!sphereData.radiusDefined) {
        std::cout << "Warning: sphere radius data not set" << std::endl;
    }
    if (!sphereData.centerDefined) {
        std::cout << "Warning: sphere center data not set" << std::endl;
    }

    sphereData.previousCenter = sphereData.currentCenter;
    sphereData.currentCenter =
        position.current + orientation.rotationMatrix * sphereData.initialCenter;
}

CollisionResult Sphere::collideWithAABB(const SphereData& s, const AABBData& a) {
    CollisionResult result;

    // find closest point on aabb to sphere
    glm::vec3 closestPoint = glm::clamp(s.currentCenter, a.currentCenter - a.halfExtents,
                                        a.currentCenter + a.halfExtents);

    // Calculate the distance between the sphere center and the closest point on the AABB
    float distance = glm::distance(s.currentCenter, closestPoint);

    // If the distance is less than or equal to the sphere's radius, they intersect
    if (distance < s.radius) {
        result.collided = true;
    }

    return result;
}

CollisionResult Sphere::collideWithTriangle(const SphereData& s, const TriangleData& t) {
    CollisionResult result;

    glm::vec3 closestPoint = TriangleGeometry::closestPointOnTriangle(t, s.currentCenter);
    float distanceSquared = glm::distance2(closestPoint, s.currentCenter);

    // std::cout << "distance: " << glm::sqrt(distanceSquared)
    //           << " current center: " << glm::to_string(s.currentCenter)
    //           << " closest point: " << glm::to_string(closestPoint) << std::endl;

    if (distanceSquared <= s.radius * s.radius) {
        result.collided = true;
    }

    return result;
}

CollisionResult Sphere::collideWithOBB(const SphereData& s, const OBBData& o) {
    CollisionResult result;

    glm::vec3 closestPoint = o.currentCorners[0];
    float minDistSq = std::numeric_limits<float>::max();

    // Check vertices
    for (const auto& corner : o.currentCorners) {
        float distSq = glm::distance2(s.currentCenter, corner);
        if (distSq < minDistSq) {
            minDistSq = distSq;
            closestPoint = corner;
        }
    }

    // Check edges
    for (int i = 0; i < 4; ++i) {
        glm::vec3 edgePoint = GeometryObject::closestPointOnLine(
            s.currentCenter, o.currentCorners[i], o.currentCorners[(i + 1) % 4]);
        float distSq = glm::distance2(s.currentCenter, edgePoint);
        if (distSq < minDistSq) {
            minDistSq = distSq;
            closestPoint = edgePoint;
        }

        edgePoint = GeometryObject::closestPointOnLine(s.currentCenter, o.currentCorners[i],
                                                       o.currentCorners[i + 4]);
        distSq = glm::distance2(s.currentCenter, edgePoint);
        if (distSq < minDistSq) {
            minDistSq = distSq;
            closestPoint = edgePoint;
        }

        edgePoint = GeometryObject::closestPointOnLine(s.currentCenter, o.currentCorners[i + 4],
                                                       o.currentCorners[((i + 1) % 4) + 4]);
        distSq = glm::distance2(s.currentCenter, edgePoint);
        if (distSq < minDistSq) {
            minDistSq = distSq;
            closestPoint = edgePoint;
        }
    }

    if (minDistSq <= s.radius * s.radius) {
        result.collided = true;
    }

    return result;
}

CollisionResult Sphere::collideWithSphere(const SphereData& s, const SphereData& o) {
    CollisionResult result;

    // Calculate the square of the distance between the centers
    float distanceSquared = glm::distance2(o.currentCenter, s.currentCenter);

    // Calculate the square of the sum of the radii
    float radiusSquared = (o.radius + s.radius) * (o.radius + s.radius);

    // If the squared distance is less than or equal to the squared sum of radii, the spheres
    // intersect
    if (distanceSquared <= radiusSquared) {
        result.collided = true;
    }

    return result;
}

CollisionResult Sphere::collideWithConvexHull(const ShapeData& sphere, const ShapeData& hull) {
    // const auto& s = std::get<SphereData>(sphere.data);
    // const auto& h = std::get<ConvexHullData>(hull.data);

    CollisionResult result;

    // // Test face normals
    // for (const auto& face : h.currentFaces) {
    //     glm::vec3 normal = face->normal;
    //     float sphereProjection = glm::dot(s.currentCenter, normal) + s.radius;
    //     float hullMin = std::numeric_limits<float>::max();
    //     float hullMax = std::numeric_limits<float>::lowest();

    //     for (const auto& vertex : h.vertices) {
    //         float projection = glm::dot(vertex, normal);
    //         hullMin = std::min(hullMin, projection);
    //         hullMax = std::max(hullMax, projection);
    //     }

    //     if (sphereProjection < hullMin || sphereProjection > hullMax) {
    //         return result;  // Separating axis found
    //     }
    // }

    // // Test sphere center to hull vertex axes
    // for (const auto& vertex : h.vertices) {
    //     glm::vec3 axis = vertex - s.currentCenter;
    //     float distance = glm::length(axis);
    //     if (distance > s.radius) {
    //         axis /= distance;  // Normalize
    //         float sphereProjection = s.radius;
    //         float hullProjection = 0.0f;

    //         for (const auto& v : h.vertices) {
    //             hullProjection = std::max(hullProjection, glm::dot(v - s.currentCenter, axis));
    //         }

    //         if (sphereProjection < hullProjection) {
    //             return result;  // Separating axis found
    //         }
    //     }
    // }
    // result.collided = true;
    return result;
}

void Sphere::populateContactManifoldWithOBB(const SphereData& s, const OBBData& o,
                                            ContactManifold& manifold,
                                            const CollisionResult& result) {
    // Implementation
}

void Sphere::populateContactManifoldWithTriangle(const SphereData& s, const TriangleData& t,
                                                 ContactManifold& manifold,
                                                 const CollisionResult& result) {
    // Closest point on triangle to sphere center
    glm::vec3 closest = TriangleGeometry::closestPointOnTriangle(t, s.currentCenter);
    glm::vec3 toSphere = s.currentCenter - closest;
    float distance = glm::length(toSphere);

    // Radial normal (closest point -> sphere center) is correct by construction and
    // doesn't depend on mesh winding; only fall back to the triangle's face normal in
    // the degenerate case where the sphere center sits exactly on the surface.
    glm::vec3 normal = distance > 1e-6f ? toSphere / distance : t.normal;

    float penetration = s.radius - distance;
    if (penetration <= 0.0f) return;  // no contact

    // Contact point on the sphere
    glm::vec3 contactOnSphere = s.currentCenter - normal * s.radius;

    // CollisionManager folds every overlapping terrain triangle under this sphere into one
    // shared manifold, calling this function once per triangle. A sphere only ever has a single
    // true contact point against a locally flat surface, so if several adjacent triangles each
    // contribute their own (slightly offset) contact point, the resolver ends up applying
    // friction impulses at multiple different points around the sphere's surface in the same
    // solve. Each of those is individually fine (radial r, torque confined to the correct
    // rolling axis), but summed together they produce a net torque around the normal axis —
    // visible as the sphere spinning in place like a top. Keep only the deepest contact so a
    // sphere is always resolved against a single point.
    if (!manifold.contacts.empty()) {
        ContactPoint& existing = manifold.contacts[0];
        if (penetration > existing.penetrationDepth) {
            existing.position = contactOnSphere;
            existing.normal = normal;
            existing.penetrationDepth = penetration;
        }
        return;
    }

    manifold.addContactPoint(contactOnSphere, normal, penetration);
}

void Sphere::populateContactManifoldWithSphere(const SphereData& s, const SphereData& o,
                                               ContactManifold& manifold,
                                               const CollisionResult& result) {
    glm::vec3 delta = o.currentCenter - s.currentCenter;
    float distance = glm::length(delta);
    if (distance == 0.0f) return;  // Prevent division by zero

    float penetration = o.radius + s.radius - distance;
    if (penetration <= 0.0f) return;  // no contact

    glm::vec3 normal = distance > 0 ? delta / distance : glm::vec3(1, 0, 0);
    glm::vec3 contactOnSphere = s.currentCenter + normal * (s.radius - 0.5f * penetration);

    manifold.addContactPoint(contactOnSphere, normal, penetration);
}

// ---- Helpers (unchanged) ----

float sweepSpherePoint(const glm::vec3& start, const glm::vec3& dir, const glm::vec3& point,
                       float radius) {
    glm::vec3 m = start - point;
    float b = glm::dot(m, dir);
    float c = glm::dot(m, m) - radius * radius;

    if (c < 0.0f) return 0.0f;  // starting inside sphere
    if (b > 0.0f) return 1.0f;  // moving away

    float discr = b * b - c;
    if (discr < 0.0f) return 1.0f;  // no hit

    return -b - sqrt(discr);  // TOI
}

float sweepSphereEdge(const glm::vec3& start, const glm::vec3& dir, const glm::vec3& A,
                      const glm::vec3& B, float radius) {
    glm::vec3 AB = B - A;
    glm::vec3 AO = start - A;
    glm::vec3 dirCross = glm::cross(dir, AB);
    float len2 = glm::dot(dirCross, dirCross);

    if (len2 < 1e-8f) return 1.0f;  // parallel
    // approximate: project motion onto closest approach (simplified for clarity)
    float t = -glm::dot(glm::cross(AO, AB), dirCross) / len2;
    return glm::clamp(t, 0.0f, 1.0f);
}

CollisionResult Sphere::sphereSweepAgainstTriangle(const SphereData& s, const TriangleData& tri) {
    glm::vec3 dir = s.currentCenter - s.previousCenter;  // movement vector
    float radius = s.radius;

    CollisionResult result;

    // --- Plane test ---
    glm::vec3 planeNormal = tri.normal;
    glm::vec3 planePoint = tri.vertices[0];

    float denom = glm::dot(dir, planeNormal);
    if (fabs(denom) > 1e-6f) {
        float dist = glm::dot(planePoint - s.previousCenter, planeNormal) - radius;
        float toi = dist / denom;

        if (toi >= 0.0f && toi <= 1.0f) {
            glm::vec3 contact = s.previousCenter + dir * toi - planeNormal * radius;
            if (TriangleGeometry::isPointInTriangle(tri, contact)) {
                result.collided = true;
                result.t = toi;
                result.contactPoint.position = contact;
                result.contactPoint.normal = planeNormal;
            }
        }
    }

    // --- Edge and vertex tests ---
    for (int i = 0; i < 3; ++i) {
        glm::vec3 A = tri.vertices[i];
        glm::vec3 B = tri.vertices[(i + 1) % 3];

        // Edge sweep
        float tEdge = sweepSphereEdge(s.previousCenter, dir, A, B, radius);
        if (tEdge < result.t) {
            result.t = tEdge;
            result.collided = true;
            result.contactPoint.position = s.previousCenter + dir * tEdge;  // approximate
            result.contactPoint.normal =
                glm::normalize((s.previousCenter + dir * tEdge) - (A + B) * 0.5f);
        }

        // Vertex sweep
        float tVertex = sweepSpherePoint(s.previousCenter, dir, A, radius);
        if (tVertex < result.t) {
            result.t = tVertex;
            result.collided = true;
            result.contactPoint.position = s.previousCenter + dir * tVertex;
            result.contactPoint.normal = glm::normalize(result.contactPoint.position - A);
        }
    }

    return result;
}