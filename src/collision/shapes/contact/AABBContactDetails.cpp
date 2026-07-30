#include "AABBContactDetails.hpp"

#include "collision/ContactResolver.hpp"
#include "collision/shapes/AABB.hpp"
#include "core/PrecompiledHeader.hpp"


glm::vec3 AABBContactDetails::calculateContactNormal(const AABBData& aabb,
                                                     const glm::vec3& intersectionPoint) {
    const float NORMAL_EPSILON =
        0.0001f;  // should be a little higher just in case compared to Config::EPSILON

    const int faces[6][4] = {
        {0, 1, 2, 3},  // Front
        {4, 5, 6, 7},  // Back
        {0, 1, 5, 4},  // Top
        {2, 3, 7, 6},  // Bottom
        {0, 3, 7, 4},  // Left
        {1, 2, 6, 5}   // Right
    };

    std::array<glm::vec3, 8> currentCorners = AABBGeometry::getAABBCorners(aabb);

    for (int i = 0; i < 6; ++i) {
        glm::vec3 p0 = currentCorners[faces[i][0]];
        glm::vec3 p1 = currentCorners[faces[i][1]];
        glm::vec3 p2 = currentCorners[faces[i][2]];

        glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        float distance = glm::dot(normal, intersectionPoint - p0);

        if (std::abs(distance) < NORMAL_EPSILON) {
            // This is the face that was hit
            return normal;
        }
    }
    // If we get here, something went wrong
    return glm::vec3(0.0f);
}
