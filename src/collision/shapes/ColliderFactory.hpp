#pragma once

#include "collision/CollisionType.hpp"
#include "components/collision/ShapeData.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"
#
class ColliderFactory {
   public:
    static OBBData& addOBB(entt::registry& reg, entt::entity& e) {
        ShapeData& shapeData = reg.get<ShapeData>(e);
        shapeData.type = CollisionType::OBB;
        shapeData.owner = e;
        shapeData.data = OBBData{};
        return std::get<OBBData>(shapeData.data);
    }

    static SphereData& addSphere(entt::registry& reg, entt::entity& e) {
        ShapeData& shapeData = reg.get<ShapeData>(e);
        shapeData.type = CollisionType::Sphere;
        shapeData.owner = e;
        shapeData.data = SphereData{};
        return std::get<SphereData>(shapeData.data);
    }

    static CapsuleData& addCapsule(entt::registry& reg, entt::entity& e) {
        ShapeData& shapeData = reg.get<ShapeData>(e);
        shapeData.type = CollisionType::Capsule;
        shapeData.owner = e;
        shapeData.data = CapsuleData{};
        return std::get<CapsuleData>(shapeData.data);
    }

    static TriangleData& addTriangle(entt::registry& reg, entt::entity& e) {
        ShapeData& shapeData = reg.get<ShapeData>(e);
        shapeData.type = CollisionType::Triangle;
        shapeData.owner = e;
        shapeData.data = TriangleData{};
        return std::get<TriangleData>(shapeData.data);
    }

    static CollisionType fromString(const std::string& typeStr) {
        if (typeStr == "ConvexHull") return CollisionType::ConvexHull;
        if (typeStr == "OBB") return CollisionType::OBB;
        if (typeStr == "Sphere") return CollisionType::Sphere;
        if (typeStr == "Plane") return CollisionType::Plane;

        return CollisionType::OBB;

        throw std::invalid_argument("Unknown type: " + typeStr);
    }
};