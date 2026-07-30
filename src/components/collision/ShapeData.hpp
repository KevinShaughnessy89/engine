#pragma once

#include "collision/CollisionType.hpp"
#include "collision/QuickHull/QuickHull.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"

struct TriangleData {
    uint32_t triangleIndex = 0;
    glm::vec3 vertices[3] = {glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)};
    int indices[3] = {0, 0, 0};
    glm::vec3 normal{0.0f};
    float planeOffset = 0.f;
    glm::vec3 initialCenter{0.0f};
    glm::vec3 currentCenter{0.0f};
    glm::vec3 previousCenter{0.0f};
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

struct SphereData {
    glm::vec3 initialCenter{0.0f};
    glm::vec3 currentCenter{0.0f};
    glm::vec3 previousCenter{0.0f};
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
    float radius = 0.0f;
    bool radiusDefined = false;
    bool centerDefined = false;
};

struct OBBData {
    glm::vec3 initialCenter{0.0f};
    glm::vec3 currentCenter{0.0f};
    glm::vec3 previousCenter{0.0f};
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
    std::vector<glm::vec3> initialCorners;
    std::vector<glm::vec3> currentCorners;
    std::array<glm::vec3, 3> initialAxes{glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)};
    std::array<glm::vec3, 3> currentAxes{glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)};
    glm::vec3 halfExtents{0.0f};
};

struct PlaneData {
    glm::vec3 normal{0.0f};
    float offset = 0.0f;
    glm::vec3 pointOnPlane{0.0f};
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

struct AABBData {
    glm::vec3 initialCenter{0.0f};
    glm::vec3 currentCenter{0.0f};
    glm::vec3 previousCenter{0.0f};
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
    glm::vec3 halfExtents{0.0f};
};

struct RayData {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f};
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

struct ConvexHullData {
    glm::vec3 initialCenter{0.0f};
    glm::vec3 currentCenter{0.0f};
    glm::vec3 previousCenter{0.0f};
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

struct CapsuleData {
    glm::vec3 initialCenter{0.0f};
    glm::vec3 currentCenter{0.0f};
    glm::vec3 previousCenter{0.0f};
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
    float radius = 0.0f;
    float height = 0.0f;
    glm::vec3 axis{0.0f, 1.0f, 0.0f};
};

struct ShapeData {
    std::variant<TriangleData, SphereData, OBBData, CapsuleData> data;
    // Matches the variant's default alternative (TriangleData).
    CollisionType type = CollisionType::Triangle;
    entt::entity owner = entt::null;
    bool isStatic = false;
};

struct GeometricData {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
    std::vector<quickhull::Vector3<float>> pointCloud;
};
