#pragma once

#include "entt-main/src/entt/entt.hpp"

class ObjectForceGenerator {
   public:
    ObjectForceGenerator() {}
    virtual void updateForce(entt::registry& reg, entt::entity& e, float duration) = 0;
};