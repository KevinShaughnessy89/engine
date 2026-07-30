#include "TriangleGeometry.hpp"

#include "collision/shapes/Ray.hpp"
#include "collision/shapes/Triangle.hpp"
#include "core/PrecompiledHeader.hpp"

void TriangleGeometry::calculateNormal(TriangleData& triangle) {
    glm::vec3 edge1 = triangle.vertices[1] - triangle.vertices[0];
    glm::vec3 edge2 = triangle.vertices[2] - triangle.vertices[0];

    if (std::isnan(edge1.x) || std::isnan(edge1.y) || std::isnan(edge1.z) || std::isnan(edge2.x) ||
        std::isnan(edge2.y) || std::isnan(edge2.z)) {
        std::cerr << "Error: One of the triangle or terrain triangle edges is NaN.\n";
        triangle.normal = glm::vec3(0.0f, 1.0f, 0.0f);  // Default to up
        return;
    }

    // Use consistent winding order - this assumes counter-clockwise from outside
    triangle.normal = glm::normalize(glm::cross(edge1, edge2));
}

void TriangleGeometry::calculateTriangleMinMax(TriangleData& triangle) {
    glm::vec3 minPoint = triangle.vertices[0];
    glm::vec3 maxPoint = triangle.vertices[0];

    for (int i = 1; i < 3; ++i) {
        // Update min point
        minPoint.x = std::min(minPoint.x, triangle.vertices[i].x);
        minPoint.y = std::min(minPoint.y, triangle.vertices[i].y);
        minPoint.z = std::min(minPoint.z, triangle.vertices[i].z);
        // Update max point
        maxPoint.x = std::max(maxPoint.x, triangle.vertices[i].x);
        maxPoint.y = std::max(maxPoint.y, triangle.vertices[i].y);
        maxPoint.z = std::max(maxPoint.z, triangle.vertices[i].z);
    }
    // Set triangle min/max
    triangle.min = minPoint;
    triangle.max = maxPoint;
}

bool TriangleGeometry::isPointOnTriangleFace(const TriangleData& triangle, const glm::vec3& point) {
    glm::vec3 edge0 = triangle.vertices[1] - triangle.vertices[0];
    glm::vec3 edge1 = triangle.vertices[2] - triangle.vertices[1];
    glm::vec3 edge2 = triangle.vertices[0] - triangle.vertices[2];
    glm::vec3 c0 = point - triangle.vertices[0];
    glm::vec3 c1 = point - triangle.vertices[1];
    glm::vec3 c2 = point - triangle.vertices[2];
    glm::vec3 normal = glm::normalize(glm::cross(edge0, -edge2));
    return (glm::dot(normal, glm::cross(edge0, c0)) > 0 &&
            glm::dot(normal, glm::cross(edge1, c1)) > 0 &&
            glm::dot(normal, glm::cross(edge2, c2)) > 0);
}

bool TriangleGeometry::isPointInTriangle(const TriangleData& triangle, const glm::vec3& point) {
    glm::vec3 v0 = triangle.vertices[1] - triangle.vertices[0];
    glm::vec3 v1 = triangle.vertices[2] - triangle.vertices[0];
    glm::vec3 v2 = point - triangle.vertices[0];

    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);

    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return u >= 0.0f && v >= 0.0f && w >= 0.0f && (u + v + w <= 1.0f);
}

bool TriangleGeometry::isPointInTriangleBarycentric(const TriangleData& triangle,
                                                    const glm::vec3& barycentric) {
    constexpr float epsilon = 1e-9f;
    // std::cout << "Triangle " << barycentric << std::endl;
    bool result = barycentric.x >= -epsilon && barycentric.y >= -epsilon &&
                  barycentric.z >= -epsilon &&
                  std::abs(barycentric.x + barycentric.y + barycentric.z - 1.0f) < epsilon;
    // std::cout << "result: " << result << std::endl;
    return result;
}

bool TriangleGeometry::isPointOnTriangleEdge(const TriangleData& triangle, const glm::vec3& point) {
    const float EPSILON = 1e-6f;  // Adjust as needed
    return (
        glm::distance2(point, GeometryObject::closestPointOnLine(point, triangle.vertices[0],
                                                                 triangle.vertices[1])) < EPSILON ||
        glm::distance2(point, GeometryObject::closestPointOnLine(point, triangle.vertices[1],
                                                                 triangle.vertices[2])) < EPSILON ||
        glm::distance2(point, GeometryObject::closestPointOnLine(point, triangle.vertices[2],
                                                                 triangle.vertices[0])) < EPSILON);
}

glm::vec3 TriangleGeometry::closestPointOnEdge(const TriangleData& triangle, glm::vec3 point,
                                               int edge) {
    switch (edge) {
        case 0:
            return GeometryObject::closestPointOnLine(point, triangle.vertices[0],
                                                      triangle.vertices[1]);
        case 1:
            return GeometryObject::closestPointOnLine(point, triangle.vertices[0],
                                                      triangle.vertices[1]);
        case 2:
            return GeometryObject::closestPointOnLine(point, triangle.vertices[0],
                                                      triangle.vertices[1]);
    }
}

glm::vec3 TriangleGeometry::closestPointOnTriangle(const TriangleData& triangle,
                                                   const glm::vec3& point) {
    glm::vec3 ab = triangle.vertices[1] - triangle.vertices[0];
    glm::vec3 ac = triangle.vertices[2] - triangle.vertices[0];
    glm::vec3 ap = point - triangle.vertices[0];

    float d1 = glm::dot(ab, ap);
    float d2 = glm::dot(ac, ap);

    // Check if P in vertex region outside A
    if (d1 <= 0.0f && d2 <= 0.0f) {
        return triangle.vertices[0];
    }

    // Check if P in vertex region outside B
    glm::vec3 bp = point - triangle.vertices[1];
    float d3 = glm::dot(ab, bp);
    float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) {
        return triangle.vertices[1];
    }

    // Check if P in edge region of AB, if so return projection of P onto AB
    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        return triangle.vertices[0] + v * ab;
    }

    // Check if P in vertex region outside C
    glm::vec3 cp = point - triangle.vertices[2];
    float d5 = glm::dot(ab, cp);
    float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) {
        return triangle.vertices[2];
    }

    // Check if P in edge region of AC, if so return projection of P onto AC
    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        return triangle.vertices[0] + w * ac;
    }

    // Check if P in edge region of BC, if so return projection of P onto BC
    float va = d3 * d6 - d5 * d4;
    glm::vec3 bc = triangle.vertices[2] - triangle.vertices[1];
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return triangle.vertices[1] + w * bc;
    }

    // P inside face region. Compute Q through its barycentric coordinates
    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return triangle.vertices[0] + ab * v + ac * w;
}

glm::vec3 TriangleGeometry::worldToBarycentric(const TriangleData& triangle,
                                               const glm::vec3& worldPoint) {
    glm::vec3 v0 = triangle.vertices[1] - triangle.vertices[0];
    glm::vec3 v1 = triangle.vertices[2] - triangle.vertices[0];
    glm::vec3 v2 = worldPoint - triangle.vertices[0];

    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);

    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return glm::vec3(u, v, w);
}

float TriangleGeometry::getBarycentricHeightFromWorld(const TriangleData& triangle,
                                                      const glm::vec3& worldPoint) {
    glm::vec3 bary = TriangleGeometry::worldToBarycentric(triangle, worldPoint);
    return bary.x * triangle.vertices[0].y + bary.y * triangle.vertices[1].y +
           bary.z * triangle.vertices[2].y;
}

glm::vec3 TriangleGeometry::barycentricToWorld(const TriangleData& triangle,
                                               const glm::vec3& barycentric) {
    return triangle.vertices[0] * barycentric.x + triangle.vertices[1] * barycentric.y +
           triangle.vertices[2] * barycentric.z;
}

bool TriangleGeometry::checkVertexIntersection(const RayData& ray, const glm::vec3& vertex,
                                               float epsilon) {
    // Calculate the vector from ray origin to vertex
    glm::vec3 v = vertex - ray.origin;

    // Project this vector onto the ray direction
    float t = glm::dot(v, ray.direction);

    // If t is negative, the vertex is behind the ray origin
    if (t < 0) {
        // std::cout << "checkVertexIntersection: t is negative, the vertex is behind the ray
        // origin" << std::endl;
        return false;
    }
    // Calculate the closest point on the ray to the vertex
    glm::vec3 closestPoint = ray.origin + t * ray.direction;

    // Check if this point is close enough to the vertex
    bool distanceCheck = glm::distance(closestPoint, vertex) < epsilon;
    if (distanceCheck)
        return distanceCheck;
    else {
        // std::cout << "failed at distanceCheck - the closest point on ray is too far from the
        // vertex" << std::endl;
        return distanceCheck;
    }
}

bool TriangleGeometry::checkEdgeIntersection(const RayData& ray, const glm::vec3& v1,
                                             const glm::vec3& v2, float epsilon) {
    // Calculate edge vector
    glm::vec3 edge = glm::normalize(v2 - v1);

    // Calculate vector from ray origin to first vertex
    glm::vec3 rayToVertex = ray.origin - v1;

    // Calculate cross products
    glm::vec3 a = glm::cross(ray.direction, edge);

    // Calculate parameters
    float denom = glm::dot(a, a);

    // Check if ray is parallel to edge (non-coincident only, assumes all parallel rays are non
    // intersecting)
    if (denom < epsilon) {
        return false;
    }

    glm::vec3 b = glm::cross(rayToVertex, edge);

    // Calculate and check parameters
    float t = glm::dot(b, a) / denom;
    float u = glm::dot(rayToVertex, a) / denom;

    // Check if intersection point is on the edge
    if (t >= -epsilon && u >= -epsilon && u <= 1 + epsilon) {
        return true;
    } else {
        return false;
    }
}

float TriangleGeometry::getDistanceAlongNormal(const TriangleData& triangle, const RayData& ray) {
    glm::vec3 edge1 = triangle.vertices[1] - triangle.vertices[0];
    glm::vec3 edge2 = triangle.vertices[2] - triangle.vertices[0];

    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

    float normalDotProduct = glm::dot(normal, ray.direction);

    // ensure distance is always measured from the ray origin along the ray direction
    if (normalDotProduct > 0) {
        normal = -normal;
    }

    float d = -glm::dot(normal, triangle.vertices[0]);

    float relativeDistance = glm::dot(ray.origin, normal) + d;

    return relativeDistance;
}

glm::vec3 TriangleGeometry::getBarycentricCoordinates(const TriangleData& triangle,
                                                      const glm::vec3& cameraPosition) {
    // Extract x and z coordinates for calculation
    float x = cameraPosition.x;
    float z = cameraPosition.z;
    float x1 = triangle.vertices[1].x;
    float z1 = triangle.vertices[1].z;
    float x2 = triangle.vertices[2].x;
    float z2 = triangle.vertices[2].z;
    float x3 = triangle.vertices[3].x;
    float z3 = triangle.vertices[3].z;

    // Calculate barycentric coordinates
    float denominator = ((z2 - z3) * (x1 - x3) + (x3 - x2) * (z1 - z3));

    float u = ((z2 - z3) * (x - x3) + (x3 - x2) * (z - z3)) / denominator;
    float v = ((z3 - z1) * (x - x3) + (x1 - x3) * (z - z3)) / denominator;
    float w = 1.0f - u - v;

    glm::vec3 barycentricCoordinates = glm::vec3(u, v, w);

    return barycentricCoordinates;
}
