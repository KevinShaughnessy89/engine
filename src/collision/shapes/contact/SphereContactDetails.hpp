#pragma once
#include "core/PrecompiledHeader.hpp"
#include "collision/CollisionAliases.hpp"

class Sphere;

class SphereContactDetails {

    public:
        SphereContactDetails() {}
        glm::vec3 calculateContactNormal(const glm::vec3& intersectionPoint);    
        float calculatePenetrationDepth(const CollisionResultPtr collisionData);
        glm::vec3 getOppositeEdgeCenter(const std::vector<glm::vec3>& points, const glm::vec3& normal);
        std::vector<int> getFurthestVertices(const glm::vec3& position);
        glm::vec3 getVertexAtIndex(int index);
        // FeatureID getFeatureID(const glm::vec3& point);
};
