#pragma once
#include "core/PrecompiledHeader.hpp"

class Model;
class PhysicsObject;
class Model;

class GeometryObject {


    public:
        static glm::vec3 closestPointOnLine(const glm::vec3& point, const glm::vec3& lineStart, const glm::vec3& lineEnd);
};
