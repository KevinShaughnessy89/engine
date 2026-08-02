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

    if (body.isGrounded && body.movementIntent > 0.0f) {
        animationState.clipID = animationState.nameToID["Walk"];
        animationState.looping = true;
    } else {
        animationState.clipID = animationState.nameToID["Idle"];
        animationState.looping = true;
    }
    body.movementIntent = 0.0f;
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
    auto& kinematicBody = Registry.get<KinematicBody>(activeCharacter);
    kinematicBody.position = glm::vec3(0.0f, 10.0f, 0.0f);
}
