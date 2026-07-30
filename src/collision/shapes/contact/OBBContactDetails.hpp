#pragma once
#include "components/collision/ShapeData.hpp"
#include "core/PrecompiledHeader.hpp"


class OBB;

class OBBContactDetails {
   public:
    OBBContactDetails() {}
    // Pass the shape instead of just the data
    glm::vec3 calculateContactNormal(const OBBData& obb, const glm::vec3& intersectionPoint);
    glm::vec3 getOppositeEdgeCenter(const std::vector<glm::vec3>& points, const glm::vec3& normal);
    std::vector<int> getFurthestVertices(const glm::vec3& position);
    glm::vec3 getVertexAtIndex(int index);
    // FeatureID getFeatureID(const glm::vec3& point);
};
