#pragma once
#include "core/PrecompiledHeader.hpp"
#include "collision/CollisionAliases.hpp"

class TriangleContactDetails {
public:
    TriangleContactDetails() {}
    glm::vec3 calculateContactNormal(const glm::vec3& intersectionPoint) const;    
    float calculatePenetrationDepth(const CollisionResultPtr collisionData) const {}
    glm::vec3 getOppositeEdgeCenter(const std::vector<glm::vec3>& points, const glm::vec3& normal) const {}
    std::vector<int> getFurthestVertices(const glm::vec3& position) const {}
    glm::vec3 getVertexAtIndex(int index) const {}
    // FeatureID getFeatureID(const glm::vec3& point) const override {}
};
