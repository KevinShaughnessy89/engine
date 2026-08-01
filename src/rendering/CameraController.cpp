#include "CameraController.hpp"

#include "CameraConstants.hpp"
#include "CameraMode.hpp"
#include "components/physics/KinematicBody.hpp"
#include "components/rendering/CameraState.hpp"
#include "components/rendering/ViewState.hpp"
#include "core/Core.hpp"
#include "entt-main/src/entt/entt.hpp"

entt::entity CameraController::activeCamera = entt::null;

// Derives orientation/forward/right/up from yaw/pitch, then resolves this frame's render
// position: FIXED trails CameraState.targetEntity's KinematicBody with an offset; FREE integrates
// ViewState.deltaPos (written by MovementController) directly, with no collision.
void CameraController::update(double deltaTime) {
    auto& camState = Registry.get<CameraState>(activeCamera);
    auto& view = Registry.get<ViewState>(activeCamera);

    glm::quat yawQuat = glm::angleAxis(view.yaw, CameraConstants::WORLD_UP_AXIS);

    glm::vec3 localRight = glm::rotate(yawQuat, CameraConstants::WORLD_RIGHT_AXIS);

    glm::quat pitchQuat = glm::angleAxis(view.pitch, localRight);

    view.orientation = glm::normalize(pitchQuat * yawQuat);

    view.forward =
        glm::normalize(glm::rotate(view.orientation, CameraConstants::WORLD_FORWARD_AXIS));

    view.right = glm::normalize(glm::rotate(view.orientation, CameraConstants::WORLD_RIGHT_AXIS));

    view.up = glm::normalize(glm::cross(view.right, view.forward));

    if (camState.cameraMode == CameraMode::FIXED && camState.targetEntity != entt::null) {
        auto& targetBody = Registry.get<KinematicBody>(camState.targetEntity);
        view.position = targetBody.position - view.forward * CameraConstants::thirdPersonDistance +
                        CameraConstants::WORLD_UP_AXIS * CameraConstants::thirdPersonHeight;
    } else {
        view.position += view.deltaPos;
    }
    view.deltaPos = glm::vec3(0.0f);

    view.lookAt = view.position + view.forward;
}

void CameraController::init() {
    auto cameraEntity = Registry.create();
    Registry.emplace<CameraState>(cameraEntity);
    Registry.emplace<ViewState>(cameraEntity);
    activeCamera = cameraEntity;
}

void CameraController::toggleFreelook() {
    auto& state = Registry.get<CameraState>(CameraController::activeCamera);
    state.modeChangedFlag = true;

    if (state.cameraMode == CameraMode::FIXED) {
        state.cameraMode = CameraMode::FREE;
        std::cout << "cameraMode " << "CameraMode::FREE" << std::endl;
    } else {
        std::cout << "cameraMode " << "CameraMode::FIXED" << std::endl;
        state.cameraMode = CameraMode::FIXED;
    }
}