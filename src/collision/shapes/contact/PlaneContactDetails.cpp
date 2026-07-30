#include "PlaneContactDetails.hpp"

glm::vec3 PlaneContactDetails::getPlanePlaneIntersectionPoint(const PlaneData& first, const PlaneData& second) {
    // p = (n1 Ã— n2) Ã— (d2n1 - d1n2) / ||n1 Ã— n2||Â²
    glm::vec3 crossProduct = glm::cross(first.normal, second.normal);
    return glm::cross(crossProduct, second.offset * first.normal - first.offset * second.normal) 
           / glm::dot(crossProduct, crossProduct);
}

float PlaneContactDetails::getPenetrationDepth(const PlaneData& plane, const glm::vec3& point) {
    float seperation = glm::dot(plane.normal, point) - plane.offset;
    return std::abs(std::min(0.0f, seperation));
}
