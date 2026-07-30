#pragma once

#include "entt-main/src/entt/entt.hpp"

// Map of animation states per model
// Determine current animation based on keyboard feedback - clipID, looping
// Determine character orientation

class CharacterController {
   public:
    static entt::entity activeCharacter;

    void setActiveCharacter(entt::entity character);

    void update(float deltaTime);
};