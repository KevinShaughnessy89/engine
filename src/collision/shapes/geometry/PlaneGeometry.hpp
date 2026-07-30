#pragma once
#include "core/PrecompiledHeader.hpp"

#include "collision/shapes/geometry/GeometryObject.hpp"
#include "components/collision/ShapeData.hpp"

class Model;
class Plane;

class PlaneGeometry : GeometryObject {
   public:
    PlaneGeometry() {}
    static bool isPointOnPlane(PlaneData& plane, const glm::vec3& point);
    static glm::vec3 projectPointOntoPlane(const glm::vec3& point, const glm::vec3& planeNormal,
                                           float planeOffset);
};
