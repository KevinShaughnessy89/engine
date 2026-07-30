#pragma once

#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entity/fwd.hpp"

class ObjectForceGenerator;

struct ObjectForceRegistration {
    entt::entity entity;
    ObjectForceGenerator* fg;
};

class ObjectForceRegistry {
   public:
    std::vector<ObjectForceRegistration> ForceRegistrations;
    void applyForceOnObject(entt::entity object, ObjectForceGenerator* fg);
    void updateForces(entt::registry& reg, float duration);
    void remove(entt::entity object, ObjectForceGenerator* fg);
    void clear();
};