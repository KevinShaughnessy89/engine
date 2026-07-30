#pragma once
#include <string>
#include <variant>

#include "collision/CollisionType.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"

/*
 * Command structs are plain data describing an entity to be created, produced
 * on worker threads and consumed on the main thread by CommandBuffer. This
 * file must stay data-only (no manager includes) so any subsystem header can
 * reference the command types without include cycles.
 */

// Everything needed to build a projectile entity and its components.
struct ProjectileSpawnCommand {
    std::string modelFilepath;
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float inverseMass;
    float force;
    float linearDamping;
    float angularDamping;
    CollisionType boundingVolume;
};

using EngineCommand = std::variant<ProjectileSpawnCommand>;
