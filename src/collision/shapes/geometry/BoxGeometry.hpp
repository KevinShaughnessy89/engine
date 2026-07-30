#pragma once
#include "core/PrecompiledHeader.hpp"

#include "collision/shapes/geometry/GeometryObject.hpp"
#include "components/collision/ShapeData.hpp"

class Model;
class PhysicsObject;
class OBB;

using EdgeSet = std::pair<std::pair<int, int>, std::array<std::pair<int, int>, 2>>;

class BoxGeometry : public GeometryObject {
   public:
    BoxGeometry();

    // Helper function to project OBB onto an axis
    static void projectOBBOntoAxis(const OBBData& obb, const glm::vec3& axis, float& min, float& max);

    // Helper function to project triangle onto an axis
    static void projectTriangleOntoAxis(const glm::vec3& v0, const glm::vec3& v1,
                                        const glm::vec3& v2, const glm::vec3& axis, float& min,
                                        float& max);

    static bool projectionsOverlap(float min1, float max1, float min2, float max2, float& overlap);

    // Get the closest point on OBB to a given point
    static glm::vec3 getClosestPointOnOBB(const OBBData& obb, const glm::vec3& point);

    // Check if a point is inside the OBB
    static bool isPointInside(const OBBData& obb, const glm::vec3& point);

    // Get the penetration depth and normal for a point inside the OBB
    static void getPointPenetration(const OBBData& obb, const glm::vec3& point, glm::vec3& normal,
                                    float& penetration);

    // Clip triangle edge against OBB face
    static void clipEdgeAgainstFace(const glm::vec3& v0, const glm::vec3& v1,
                                    const glm::vec3& faceNormal, float faceDistance,
                                    std::vector<glm::vec3>& clippedPoints);

    static const std::vector<std::pair<int, int>> edgeIndices;
    static std::vector<EdgeSet> oppositeFaceIndices;
};
