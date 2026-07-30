#pragma once

#include "GravityForceGenerator.hpp"
#include "ObjectForceRegistry.hpp"
#include "components/physics/PhysicsComponents.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"

class PhysicsManager {
   public:
    GravityForceGenerator gravityForceGenerator;
    ObjectForceRegistry objectForceRegistry;
    PhysicsManager() = default;

    void init();

    void runPhysics(float duration);

    void applyGravity(entt::entity e, float duration);

    void applyProjectileForce(glm::vec3 position, glm::vec3 direction, float force,
                              entt::entity entity);

    void applyForceOnObject(entt::entity entity, const glm::vec3& force);

    void updateIitw(Orientation& o, Inertia& inertia);

    glm::vec3 localToWorldPoint(const Position& pos, const Orientation& rot, glm::vec3 localPoint);

    glm::vec3 localToWorldVector(const Orientation& rot, glm::vec3 localVector);

    glm::vec3 worldToLocalPoint(const Position& pos, const Orientation& rot, glm::vec3 worldPoint);

    glm::vec3 worldToLocalVector(const Orientation& rot, glm::vec3 worldVector);

    glm::vec3 getVelocityAtPoint(const Velocity& velocity, const glm::vec3& angularVelocity,
                                 const Position& pos, const glm::vec3& point);

    void addForce(entt::entity e, const glm::vec3& force);

    void addForceAtPoint(entt::entity e, glm::vec3 worldPoint, glm::vec3 worldForce);

    bool shouldSleep(entt::entity e, float duration);

    void update(entt::entity e, float duration);

    void integrate(entt::entity e, float duration);

    void translate(Position& position, Orientation& orientation, const glm::vec3& translation);

    void rotate(Orientation& orientation, const glm::vec3& rotationAngle);
};