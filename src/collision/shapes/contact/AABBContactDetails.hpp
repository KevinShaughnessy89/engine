#pragma once
#include "collision/CollisionAliases.hpp"
#include "components/collision/ShapeData.hpp"
#include "core/PrecompiledHeader.hpp"


class AABB;
struct CollisionResult;

class AABBContactDetails {
   public:
    AABBContactDetails() {}
    static glm::vec3 calculateContactNormal(const AABBData& aabb,
                                            const glm::vec3& intersectionPoint);
    static float calculatePenetrationDepth(const CollisionResultPtr collisionData);
    static glm::vec3 getOppositeEdgeCenter(const std::vector<glm::vec3>& points,
                                           const glm::vec3& normal);
    static std::vector<int> getFurthestVertices(const glm::vec3& position);
    static glm::vec3 getVertexAtIndex(int index);
    // FeatureID getFeatureID(const glm::vec3& point);
};
