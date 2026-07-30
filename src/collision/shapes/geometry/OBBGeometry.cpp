#include "core/PrecompiledHeader.hpp"
#include "OBBGeometry.hpp"
#include "collision/shapes/OBB.hpp"
#include "collision/shapes/geometry/OBBGeometry.hpp"

std::vector<glm::vec3> OBBGeometry::getEdges(const OBBData& obb) {
    std::vector<glm::vec3> edges;
    edges.reserve(12);

    // Bottom face
    edges.push_back(obb.currentCorners[1] - obb.currentCorners[0]); // Edge 0
    edges.push_back(obb.currentCorners[3] - obb.currentCorners[1]); // Edge 1
    edges.push_back(obb.currentCorners[2] - obb.currentCorners[3]); // Edge 2
    edges.push_back(obb.currentCorners[0] - obb.currentCorners[2]); // Edge 3

    // Top face
    edges.push_back(obb.currentCorners[5] - obb.currentCorners[4]); // Edge 4
    edges.push_back(obb.currentCorners[7] - obb.currentCorners[5]); // Edge 5
    edges.push_back(obb.currentCorners[6] - obb.currentCorners[7]); // Edge 6
    edges.push_back(obb.currentCorners[4] - obb.currentCorners[6]); // Edge 7

    // Vertical edges
    edges.push_back(obb.currentCorners[4] - obb.currentCorners[0]); // Edge 8
    edges.push_back(obb.currentCorners[5] - obb.currentCorners[1]); // Edge 9
    edges.push_back(obb.currentCorners[6] - obb.currentCorners[2]); // Edge 10
    edges.push_back(obb.currentCorners[7] - obb.currentCorners[3]); // Edge 11

    return edges;
}

void OBBGeometry::projectOBBOntoAxis(const OBBData& obb, const glm::vec3& axis, float& max, float& min) {
    min = max = glm::dot(axis, obb.currentCorners[0]);
    for (const auto& corner : obb.currentCorners) {
        float length = glm::dot(axis, corner);
        max = std::max(length, max);
        min = std::min(length, min);
    }
}

bool OBBGeometry::testAxisSeparation(const OBBData& obb, const glm::vec3& axis, const std::vector<glm::vec3>& points) {

    // Project OBB
    float boxMin = std::numeric_limits<float>::max();
    float boxMax = -std::numeric_limits<float>::max();;

    for (int i = 0; i < 8; ++i) {
        float p = glm::dot(obb.currentCorners[i], axis);
        boxMin = std::min(boxMin, p);
        boxMax = std::max(boxMax, p);
    }

    // Project points
    float pointsMin = std::numeric_limits<float>::max();
    float pointsMax = -std::numeric_limits<float>::max();

    for (const auto& point : points) {
        float p = glm::dot(point, axis);
        pointsMin = std::min(pointsMin, p);
        pointsMax = std::max(pointsMax, p);
    }

    // Check for overlap
    bool overlap = (boxMin <= pointsMax && pointsMin <= boxMax);

    return overlap;
}

std::vector<int> OBBGeometry::getFurthestVertices(const OBBData& obb, const glm::vec3& point) {
    float maxDistance = -std::numeric_limits<float>::max();
    std::vector<int> furthestCorners;

    // First pass: find the maximum distance
    for (int i = 0; i < obb.currentCorners.size(); i++) {
        float distance = glm::distance2(obb.currentCorners[i], point);  // Using distance squared for efficiency
        if (distance > maxDistance) {
            maxDistance = distance;
        }
    }

    // Second pass: collect all corners at the maximum distance
    for (int i = 0; i < obb.currentCorners.size(); i++) {
        float distance = glm::distance2(obb.currentCorners[i], point);
        if (std::abs(distance - maxDistance) < Config::EPSILON) {  // Use EPSILON for float comparison
            furthestCorners.push_back(i);
        }
    }

    return furthestCorners;
}

glm::vec3 OBBGeometry::getOppositeEdgeCenter(const OBBData& obb, const glm::vec3& vertexA, const glm::vec3& vertexB, const glm::vec3& normal) {

    int index1 = -1, index2 = -1;
    for (int i = 0; i < 8; ++i) {
        if (obb.currentCorners[i] == vertexA) index1 = i;
        if (obb.currentCorners[i] == vertexB) index2 = i;
        if (index1 != -1 && index2 != -1) break;
    }

    std::pair<int,int> currentEdge = {std::min(index1, index2), std::max(index1, index2)};    
    std::array<std::pair<int, int>, 2> oppositeEdgeSet;
    for (const auto& edgeSet : oppositeFaceIndices) {
        if (edgeSet.first == currentEdge) {
            oppositeEdgeSet = edgeSet.second;
            break;
        }
    }
    glm::vec3 pointC1 = obb.currentCorners[oppositeEdgeSet[0].first];
    glm::vec3 pointC2 = obb.currentCorners[oppositeEdgeSet[1].first];
    glm::vec3 pointA = obb.currentCorners[index1];
    glm::vec3 pointB = obb.currentCorners[index2];
    glm::vec3 normalFace1 = glm::normalize(glm::cross( (pointC1 - pointA), (pointB - pointA)));
    glm::vec3 normalFace2 = glm::normalize(glm::cross( (pointC2 - pointA), (pointB - pointA)));

    if (glm::dot(normalFace1, normal) < glm::dot(normalFace2, normal)) {
        return (obb.currentCorners[oppositeEdgeSet[1].first] + obb.currentCorners[oppositeEdgeSet[1].second]) * 0.5f;
    } else {
        return (obb.currentCorners[oppositeEdgeSet[0].first] + obb.currentCorners[oppositeEdgeSet[0].second]) * 0.5f;
    }
}

glm::vec3 OBBGeometry::calculateLocalCoordinates(const OBBData& obb, const glm::vec3& P) {
    // Calculate the box's local axes in world space
    glm::vec3 xAxis = glm::normalize(obb.currentCorners[1] - obb.currentCorners[0]);
    glm::vec3 yAxis = glm::normalize(obb.currentCorners[2] - obb.currentCorners[0]);
    glm::vec3 zAxis = glm::normalize(obb.currentCorners[4] - obb.currentCorners[0]);
    
    // Calculate the point relative to the box's minimum corner (corner 0)
    glm::vec3 relativePoint = P - obb.currentCorners[0];
    
    // Project the relative point onto each axis and normalize
    float u = glm::dot(relativePoint, xAxis) / (2 * obb.halfExtents.x);
    float v = glm::dot(relativePoint, yAxis) / (2 * obb.halfExtents.y);
    float w = glm::dot(relativePoint, zAxis) / (2 * obb.halfExtents.z);
    
    return glm::vec3(u, v, w);
}

bool OBBGeometry::containsPoint(const OBBData& obb, const glm::vec3& point) {
    return (point.x >= obb.min.x && point.x <= obb.max.x &&
            point.y >= obb.min.y && point.y <= obb.max.y &&
            point.z >= obb.min.z && point.z <= obb.max.z);
}
