#pragma once
#include "components/collision/ShapeData.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"

class OBB;
class ConvexHull;
class Sphere;

struct PhysicsProperties {
    glm::mat3 inertiaTensor;
    glm::mat3 principleAxes;
    glm::vec3 centroid;
};

class PhysicsPropertiesCalculatorBase {
   public:
    virtual ~PhysicsPropertiesCalculatorBase() = default;
    virtual PhysicsProperties calculateBaseProperties(entt::entity& e) = 0;

    // Add other pure virtual functions as needed
};

// Primary template declaration
template <typename BoundingCollisionType>
class PhysicsPropertiesCalculator : public PhysicsPropertiesCalculatorBase {
   private:
    BoundingCollisionType* bv;

   public:
    PhysicsPropertiesCalculator(BoundingCollisionType* boundingVolume) : bv(boundingVolume) {}
    PhysicsProperties calculateBaseProperties(entt::entity& e) override;
};

// Template specializations
template <>
class PhysicsPropertiesCalculator<Sphere> : public PhysicsPropertiesCalculatorBase {
   private:
    Sphere* bv;

   public:
    PhysicsPropertiesCalculator(Sphere* boundingVolume) : bv(boundingVolume) {}
    PhysicsProperties calculateBaseProperties(entt::entity& e) override;
    glm::mat3 calculateInertiaTensorLocal(entt::entity& e);
    glm::mat3 calculatePrincipleAxes(glm::mat3 inertiaTensor);
    glm::vec3 calculateCentroid(entt::entity& e);
};

template <>
class PhysicsPropertiesCalculator<OBB> : public PhysicsPropertiesCalculatorBase {
   private:
    OBBData& bv;

   public:
    PhysicsPropertiesCalculator(OBBData& boundingVolume) : bv(boundingVolume) {}

    PhysicsProperties calculateBaseProperties(entt::entity& e) override;
    glm::mat3 calculateInertiaTensorLocal(entt::entity& e);
    glm::mat3 calculatePrincipleAxes(glm::mat3 inertiaTensor);

    glm::vec3 calculateCentroid(entt::entity& e);
};

template <>
class PhysicsPropertiesCalculator<ConvexHull> : public PhysicsPropertiesCalculatorBase {
   private:
    ConvexHullData& bv;

   public:
    PhysicsPropertiesCalculator(ConvexHullData& boundingVolume) : bv(boundingVolume) {}
    PhysicsProperties calculateBaseProperties(entt::entity& e) override;
    glm::mat3 calculateInertiaTensorLocal(entt::entity& e);
    glm::mat3 calculatePrincipleAxes(glm::mat3 inertiaTensor);
    glm::vec3 calculateCentroid(entt::entity& e);
};