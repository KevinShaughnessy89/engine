#include "core/PrecompiledHeader.hpp"
#ifndef _GRAVITYFORCEGENERATOR
#define _GRAVITYFORCEGENERATOR

#include "ObjectForceGenerator.hpp"
class DynamicObject;

class GravityForceGenerator : public ObjectForceGenerator {
   public:
    GravityForceGenerator() {}
    glm::vec3 gravity = glm::vec3(0.0f, -1.0f, 0.0f);
    void updateForce(entt::registry& reg, entt::entity& e, float duration) override;
};

#endif
