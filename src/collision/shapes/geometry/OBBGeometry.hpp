#pragma once
#include "core/PrecompiledHeader.hpp"

#include "BoxGeometry.hpp"
#include "components/collision/ShapeData.hpp"

class OBBGeometry : public BoxGeometry {

    public:

        OBBGeometry() {}

        static std::vector<glm::vec3> getEdges(const OBBData& obb);
        static void projectOBBOntoAxis(const OBBData& obb, const glm::vec3& axis, float& max, float& min);
        static bool testAxisSeparation(const OBBData& obb, const glm::vec3& axis, const std::vector<glm::vec3>& points);
        static std::vector<int> getFurthestVertices(const OBBData& obb, const glm::vec3& point);
        static glm::vec3 calculateLocalCoordinates(const OBBData& obb, const glm::vec3& P);
        static glm::vec3 getOppositeEdgeCenter(const OBBData& obb, const glm::vec3& vertexA, const glm::vec3& vertexB, const glm::vec3& normal);
        static bool containsPoint(const OBBData& obb, const glm::vec3& point);

};
