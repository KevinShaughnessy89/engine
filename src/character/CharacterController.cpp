#include "CharacterController.hpp"

#include "components/physics/KinematicBody.hpp"
#include "components/rendering/AnimationState.hpp"
#include "components/rendering/CameraState.hpp"
#include "core/Core.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/ModelLoader.hpp"

entt::entity CharacterController::activeCharacter = entt::null;

void CharacterController::setActiveCharacter(entt::entity character) {
    activeCharacter = character;
}
void CharacterController::update(float deltaTime) {
    auto [body, animationState] = Registry.get<KinematicBody, AnimationState>(activeCharacter);

    if (body.isGrounded && glm::length(glm::vec2(body.velocity.x, body.velocity.z)) > 0.1f) {
        animationState.clipID = animationState.nameToID["Walk"];
        animationState.looping = true;
    } else {
        animationState.clipID = animationState.nameToID["Idle"];
        animationState.looping = true;
    }
}

void CharacterController::createNewCharacter(const std::string& name) {
    auto character = ModelRegistry.getEntity(name);  // model only
    Registry.emplace<KinematicBody>(character);
    if (activeCharacter == entt::null) {
        activeCharacter = character;
        Registry.get<CameraState>(CameraController::activeCamera).targetEntity = character;
    }
}

void CharacterController::init() {
    createNewCharacter("Character");
}
