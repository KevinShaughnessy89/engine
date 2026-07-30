#pragma once
#include "core/PrecompiledHeader.hpp"

#include "BoxGeometry.hpp"

class AABB;

class AABBGeometry: public BoxGeometry {
    public:

        AABBGeometry() {}
        static void projectAABBOntoAxis(const AABBData& aabb, const glm::vec3& axis, float& min, float& max);
        static std::array<glm::vec3, 8> getAABBCorners(const AABBData& aabb);
        static bool containsPoint(const AABBData& aabb, const glm::vec3& point);
};
