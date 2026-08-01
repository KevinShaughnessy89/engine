#pragma once

#include "entt-main/src/entt/entt.hpp"

// Map of animation states per model
// Determine current animation based on keyboard feedback - clipID, looping
// Determine character orientation

struct CharacterController {
   public:
    static entt::entity activeCharacter;

    void setActiveCharacter(entt::entity character);
    void createNewCharacter(const std::string& name);
    void init();
    void update(float deltaTime);
};