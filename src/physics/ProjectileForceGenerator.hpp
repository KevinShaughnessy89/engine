#pragma once

#include "core/PrecompiledHeader.hpp"
#include "ObjectForceGenerator.hpp"
#include "entt-main/src/entt/entt.hpp"

class ProjectileForceGenerator : public ObjectForceGenerator {

    public:
        ProjectileForceGenerator(glm::vec3 position, glm::vec3 direction, float force) :
        position(position),
        direction(direction),
        force(force) {
            projectileForce = glm::normalize(direction) * force;
        }
        glm::vec3 position;
        glm::vec3 direction;
        float force;
        glm::vec3 projectileForce;
        void updateForce(entt::registry& reg, entt::entity& e, float duration);        
};