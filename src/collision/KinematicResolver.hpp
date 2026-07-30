#pragma once

#include "ResolvedConstraint.hpp"
#include "components/physics/KinematicBody.hpp"
#include "entt-main/src/entt/entt.hpp"

struct KinematicResolverConfig {
    float thresholdDownSlope = 0.7f;
    float thresholdUpSlope = 1.3f;
};

class KinematicResolver {
   public:
    // Terrain grounding samples the chunk's baked heightmap; candidates only ground entity shapes.
    static ResolvedConstraint resolveCollisionConstraint(glm::vec3 position,
                                                         const std::vector<uint32_t>& candidates,
                                                         const KinematicBody& body,
                                                         double deltaTime);
    static ResolvedConstraint fromHeightMap(glm::vec3 position, const KinematicBody& body,
                                            double deltaTime);
    static bool checkIsGrounded(const glm::vec3& normal);
};