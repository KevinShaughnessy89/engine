#pragma once
#include "core/PrecompiledHeader.hpp"

class ContactGeometry {
    public:
        virtual glm::vec3 calculateContactNormal(const glm::vec3 intersectionPoint) const = 0;
        virtual std::vector<int> getFurthestVertices(const glm::vec3& point) = 0;
        virtual glm::vec3 getVertexAtIndex(int index) = 0;
        virtual glm::vec3 getOppositeEdgeCenter(std::pair<glm::vec3, glm::vec3> edge, glm::vec3 normal) = 0;
};
