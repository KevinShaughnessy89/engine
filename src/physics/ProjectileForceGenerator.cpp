#include "ProjectileForceGenerator.hpp"

#include "PhysicsManager.hpp"
#include "components/physics/Position.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"


void ProjectileForceGenerator::updateForce(entt::registry& reg, entt::entity& e, float duration) {
    auto& position = reg.get<Position>(e);
    Physics.addForceAtPoint(e, position.current, projectileForce);
}