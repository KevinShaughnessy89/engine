#include "Ray.hpp"

#include "collision/CollisionResult.hpp"
#include "geometry/TriangleGeometry.hpp"

CollisionResult Ray::intersectTriangle(const RayData& ray, const TriangleData& triangle) {
    const float EPSILON = 1e-6f;
    const float BARY_EPS = 1e-4f;

    CollisionResult result;

    const glm::vec3& v0 = triangle.vertices[0];
    const glm::vec3& v1 = triangle.vertices[1];
    const glm::vec3& v2 = triangle.vertices[2];

    glm::vec3 rayDir =
        glm::normalize(ray.direction);  // Might be wasted computation if the ray direction is
                                        // always along the y axis (already normalized, (0,-1,0))
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;

    glm::vec3 pVec = glm::cross(rayDir, edge2);
    float det = glm::dot(edge1, pVec);

    if (std::abs(det) < EPSILON) {
        return result;  // Ray is parallel to the triangle
    }

    float invDet = 1.0f / det;
    glm::vec3 tVec = ray.origin - v0;
    float u = invDet * glm::dot(tVec, pVec);

    if (u < -BARY_EPS || u > 1.0f + BARY_EPS) {
        return result;
    }

    glm::vec3 qVec = glm::cross(tVec, edge1);
    float v = invDet * glm::dot(rayDir, qVec);

    if (v < -BARY_EPS || (u + v) > 1.0f + BARY_EPS) {
        return result;
    }

    float t = invDet * glm::dot(edge2, qVec);

    if (t > EPSILON) {
        result.collided = true;
        result.t = t;
        result.contactPoint.position = ray.origin + rayDir * t;
        result.contactPoint.normal = triangle.normal;
    }

    return result;
}

CollisionResult Ray::intersectTriangleStraightDown(const RayData& ray, const TriangleData& tri) {
    glm::vec3 N = tri.normal;  // assume unit length, precomputed or cross(B-A,C-A) normalized
    if (N.y < 0) N.y = -N.y;
    float maxDist = 100.f;
    CollisionResult result;

    float denom = glm::dot(N, glm::vec3(0.f, -1.f, 0.f));
    if (glm::abs(denom) < 1e-6f) {
        result.collided = false;
        return result;
    }

    float t = glm::dot(tri.vertices[0] - ray.origin, N) / denom;

    if (t < 0.f || t > maxDist) {
        result.collided = false;
        return result;  // plane hit is behind ray or beyond your probe distance
    }

    glm::vec3 point = ray.origin + glm::vec3(0.f, -1.f, 0.f) * t;

    if (!TriangleGeometry::isPointInTriangle(tri, point)) {
        result.collided = false;
        return result;  // hit the infinite plane, but outside the triangle's edges
    }

    result.collided = true;
    result.contactPoint.position = point;
    return result;
}

CollisionResult Ray::intersectAABB(const glm::vec3& min, const glm::vec3& max) const {
    CollisionResult result;

    // glm::vec3 invDir = 1.0f / direction;
    // glm::vec3 t_min = (min - origin) * invDir;
    // glm::vec3 t_max = (max - origin) * invDir;

    // glm::vec3 t1 = glm::min(t_min, t_max);
    // glm::vec3 t2 = glm::max(t_min, t_max);

    // float t_near = glm::max(glm::max(t1.x, t1.y), t1.z);
    // float t_far = glm::min(glm::min(t2.x, t2.y), t2.z);

    // std::cout << "Ray origin: " << glm::to_string(origin) << std::endl;
    // std::cout << "Ray direction: " << glm::to_string(direction) << std::endl;
    // std::cout << "AABB min: " << glm::to_string(min) << std::endl;
    // std::cout << "AABB max: " << glm::to_string(max) << std::endl;
    // std::cout << "t_near: " << t_near << ", t_far: " << t_far << std::endl;

    // result->collided = t_near <= t_far && t_far > 0;

    return result;
}