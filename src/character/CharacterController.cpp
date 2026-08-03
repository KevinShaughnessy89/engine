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
// body.speed is tuned as a units/second value for KinematicResolver's collision step clamping
// (see KinematicResolver.cpp), not as an animation multiplier -- baseWalkSpeed converts it into a
// playbackSpeed ratio so 1.0 corresponds to the Walk clip's natural (default) speed.
constexpr float baseWalkSpeed = 4000.0f;

void CharacterController::update(float deltaTime) {
    auto [body, animationState] = Registry.get<KinematicBody, AnimationState>(activeCharacter);

    if (body.isGrounded && body.movementDirection != MovementDirection::NONE) {
        bool forward = body.movementDirection == MovementDirection::FORWARD;
        std::string clipName = body.movementState == MovementState::RUN
                                    ? (forward ? "Run-Forwards" : "Run-Backwards")
                                    : (forward ? "Walk-Forwards" : "Walk-Backwards");
        animationState.clipID = animationState.nameToID[clipName];
        animationState.looping = true;
        animationState.playbackSpeed = body.speed / baseWalkSpeed;
    } else {
        animationState.clipID = animationState.nameToID["Idle"];
        animationState.looping = true;
        animationState.playbackSpeed = 1.0f;
    }
    body.movementDirection = MovementDirection::NONE;
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
