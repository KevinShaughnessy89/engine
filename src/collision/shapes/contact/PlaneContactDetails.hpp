#pragma once
#include "core/PrecompiledHeader.hpp"
#include "components/collision/ShapeData.hpp"

class Plane;

class PlaneContactDetails {
public:
    PlaneContactDetails() {}
    // FeatureID getFeatureID(const glm::vec3& point);
    static glm::vec3 getPlanePlaneIntersectionPoint(const PlaneData& first, const PlaneData& second);
    static float getPenetrationDepth(const PlaneData& plane, const glm::vec3& point);
};