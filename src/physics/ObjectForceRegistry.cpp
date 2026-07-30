#include "ObjectForceRegistry.hpp"

#include "core/PrecompiledHeader.hpp"
#include "physics/ObjectForceGenerator.hpp"

void ObjectForceRegistry::applyForceOnObject(entt::entity object, ObjectForceGenerator* fg) {
    ObjectForceRegistration temp{object, fg};
    ForceRegistrations.push_back(temp);
}

void ObjectForceRegistry::updateForces(entt::registry& reg, float duration) {
    auto it = ForceRegistrations.begin();

    while (it != ForceRegistrations.end()) {
        it->fg->updateForce(reg, it->entity, duration);
        ForceRegistrations.erase(it);
        it = ForceRegistrations.begin();  // Start from the beginning each time
    }
}
