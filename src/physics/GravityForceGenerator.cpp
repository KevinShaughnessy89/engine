#include "GravityForceGenerator.hpp"

#include "PhysicsManager.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"


void GravityForceGenerator::updateForce(entt::registry& reg, entt::entity& e, float duration) {
    auto& inverseMass = reg.get<InverseMass>(e);

    Physics.addForce(e, gravity / inverseMass.value);
}
