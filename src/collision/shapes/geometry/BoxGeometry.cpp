#include "core/PrecompiledHeader.hpp"
#include "BoxGeometry.hpp"
#include "collision/shapes/OBB.hpp"

std::vector<EdgeSet> BoxGeometry::oppositeFaceIndices = {
    {std::make_pair(0, 1),
     std::array<std::pair<int, int>, 2>{std::make_pair(4, 5), std::make_pair(2, 3)}},
    {std::make_pair(2, 3),
     std::array<std::pair<int, int>, 2>{std::make_pair(0, 1), std::make_pair(6, 7)}},
    {std::make_pair(2, 0),
     std::array<std::pair<int, int>, 2>{std::make_pair(3, 1), std::make_pair(6, 4)}},
    {std::make_pair(3, 1),
     std::array<std::pair<int, int>, 2>{std::make_pair(2, 0), std::make_pair(7, 5)}},
    {std::make_pair(2, 6),
     std::array<std::pair<int, int>, 2>{std::make_pair(4, 0), std::make_pair(3, 7)}},
    {std::make_pair(7, 3),
     std::array<std::pair<int, int>, 2>{std::make_pair(5, 1), std::make_pair(6, 2)}},
    {std::make_pair(1, 5),
     std::array<std::pair<int, int>, 2>{std::make_pair(3, 7), std::make_pair(0, 4)}},
    {std::make_pair(4, 0),
     std::array<std::pair<int, int>, 2>{std::make_pair(1, 5), std::make_pair(2, 6)}},
    {std::make_pair(6, 4),
     std::array<std::pair<int, int>, 2>{std::make_pair(7, 5), std::make_pair(2, 0)}},
    {std::make_pair(6, 7),
     std::array<std::pair<int, int>, 2>{std::make_pair(4, 5), std::make_pair(2, 3)}},
    {std::make_pair(7, 5),
     std::array<std::pair<int, int>, 2>{std::make_pair(6, 4), std::make_pair(3, 1)}},
    {std::make_pair(5, 4),
     std::array<std::pair<int, int>, 2>{std::make_pair(0, 1), std::make_pair(6, 7)}}};

const std::vector<std::pair<int, int>> BoxGeometry::edgeIndices = {
    {0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

void BoxGeometry::projectOBBOntoAxis(const OBBData& obb, const glm::vec3& axis, float& min,
                                     float& max) {
    float centerProjection = glm::dot(obb.currentCenter, axis);
    float radiusProjection = 0.0f;

    for (int i = 0; i < 3; i++) {
        radiusProjection += std::abs(glm::dot(obb.currentAxes[i], axis)) * obb.halfExtents[i];
    }

    min = centerProjection - radiusProjection;
    max = centerProjection + radiusProjection;
}

void BoxGeometry::projectTriangleOntoAxis(const glm::vec3& v0, const glm::vec3& v1,
                                          const glm::vec3& v2, const glm::vec3& axis, float& min,
                                          float& max) {
    float proj0 = glm::dot(v0, axis);
    float proj1 = glm::dot(v1, axis);
    float proj2 = glm::dot(v2, axis);

    min = std::min({proj0, proj1, proj2});
    max = std::max({proj0, proj1, proj2});
}

bool BoxGeometry::projectionsOverlap(float min1, float max1, float min2, float max2,
                                     float& overlap) {
    if (max1 < min2 || max2 < min1) {
        return false;
    }

    float overlap1 = max1 - min2;
    float overlap2 = max2 - min1;
    overlap = std::min(overlap1, overlap2);
    return true;
}

glm::vec3 BoxGeometry::getClosestPointOnOBB(const OBBData& obb, const glm::vec3& point) {
    glm::vec3 d = point - obb.currentCenter;
    glm::vec3 result = obb.currentCenter;

    for (int i = 0; i < 3; i++) {
        float dist = glm::dot(d, obb.currentAxes[i]);
        dist = std::max(-obb.halfExtents[i], std::min(obb.halfExtents[i], dist));
        result += obb.currentAxes[i] * dist;
    }

    return result;
}

bool BoxGeometry::isPointInside(const OBBData& obb, const glm::vec3& point) {
    glm::vec3 d = point - obb.currentCenter;

    for (int i = 0; i < 3; i++) {
        float dist = std::abs(glm::dot(d, obb.currentAxes[i]));
        if (dist > obb.halfExtents[i]) {
            return false;
        }
    }
    return true;
}

void BoxGeometry::getPointPenetration(const OBBData& obb, const glm::vec3& point, glm::vec3& normal,
                                      float& penetration) {
    glm::vec3 d = point - obb.currentCenter;
    float minPenetration = std::numeric_limits<float>::max();

    for (int i = 0; i < 3; i++) {
        float dist = glm::dot(d, obb.currentAxes[i]);
        float pen1 = obb.halfExtents[i] - dist;  // Penetration to positive face
        float pen2 = obb.halfExtents[i] + dist;  // Penetration to negative face

        if (pen1 < minPenetration) {
            minPenetration = pen1;
            normal = obb.currentAxes[i];
        }

        if (pen2 < minPenetration) {
            minPenetration = pen2;
            normal = -obb.currentAxes[i];
        }
    }
    penetration = minPenetration;
}

void BoxGeometry::clipEdgeAgainstFace(const glm::vec3& v0, const glm::vec3& v1,
                                      const glm::vec3& faceNormal, float faceDistance,
                                      std::vector<glm::vec3>& clippedPoints) {
    float d0 = glm::dot(v0, faceNormal) - faceDistance;
    float d1 = glm::dot(v1, faceNormal) - faceDistance;

    if (d0 >= 0 && d1 >= 0) {
        return;  // both outside
    }

    if (d0 < 0 && d1 < 0) {
        clippedPoints.push_back(v1);  // both inside
        return;
    }

    // Edge crosses the face
    float t = d0 / (d0 - d1);
    glm::vec3 intersection = v0 + (v1 - v0) * t;

    if (d1 < 0) {
        clippedPoints.push_back(intersection);
        clippedPoints.push_back(v1);
    } else {
        clippedPoints.push_back(intersection);
    }
}
