#include "CharacterController.hpp"

#include "character/AnimationRenderer.hpp"
#include "components/physics/KinematicBody.hpp"
#include "components/rendering/AnimationRegistryComponent.hpp"
#include "components/rendering/AnimationState.hpp"
#include "core/Core.hpp"


void CharacterController::setActiveCharacter(entt::entity character) {
    activeCharacter = character;
}
void CharacterController::update(float deltaTime) {
    auto [body, animationState, animationRegistry] =
        Registry.get<KinematicBody, AnimationState, AnimationRegistryComponent>(activeCharacter);

    if (body.isGrounded && glm::length(glm::vec2(body.velocity.x, body.velocity.z)) > 0.1f) {
        animationState.clipID = animationRegistry.registry->nameToID["walk"];
        animationState.looping = true;
    } else {
        animationState.clipID = animationRegistry.registry->nameToID["idle"];
        animationState.looping = true;
    }
}