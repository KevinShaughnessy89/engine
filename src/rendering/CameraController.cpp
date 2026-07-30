#include "CameraController.hpp"

#include "CameraConstants.hpp"
#include "components/physics/KinematicBody.hpp"
#include "components/rendering/CameraState.hpp"
#include "components/rendering/ViewState.hpp"
#include "core/Core.hpp"
#include "entt-main/src/entt/entt.hpp"

entt::entity CameraController::activeCamera = entt::null;

// Purely ViewState: derive orientation/forward/right/up from yaw/pitch for whatever's looking
// through this camera. Movement, gravity, and collision resolution live on KinematicBody and are
// handled by KinematicController -- this only produces the direction that body is facing.
void CameraController::update(double deltaTime) {
    auto entityView = Registry.view<ViewState>();

    for (auto& cameraEntity : entityView) {
        auto& view = Registry.get<ViewState>(cameraEntity);

        glm::quat yawQuat = glm::angleAxis(view.yaw, CameraConstants::WORLD_UP_AXIS);

        glm::vec3 localRight = glm::rotate(yawQuat, CameraConstants::WORLD_RIGHT_AXIS);

        glm::quat pitchQuat = glm::angleAxis(view.pitch, localRight);

        view.orientation = glm::normalize(pitchQuat * yawQuat);

        view.forward =
            glm::normalize(glm::rotate(view.orientation, CameraConstants::WORLD_FORWARD_AXIS));

        view.right =
            glm::normalize(glm::rotate(view.orientation, CameraConstants::WORLD_RIGHT_AXIS));

        view.up = glm::normalize(glm::cross(view.right, view.forward));
    }
}

void CameraController::init() {
    auto cameraEntity = Registry.create();
    Registry.emplace<CameraState>(cameraEntity);
    Registry.emplace<KinematicBody>(cameraEntity);
    Registry.emplace<ViewState>(cameraEntity);
    activeCamera = cameraEntity;
}

CameraState& CameraController::getCameraState(entt::entity cameraEntity) {
    return Registry.get<CameraState>(cameraEntity);
}

KinematicBody& CameraController::getKinematicBody(entt::entity cameraEntity) {
    return Registry.get<KinematicBody>(cameraEntity);
}

ViewState& CameraController::getViewState(entt::entity cameraEntity) {
    return Registry.get<ViewState>(cameraEntity);
}

void CameraController::toggleFreelook() {
    auto& state = CameraController::getCameraState(CameraController::activeCamera);
    state.modeChangedFlag = true;

    if (state.cameraMode == CameraMode::FIXED) {
        state.cameraMode = CameraMode::FREE;
        std::cout << "cameraMode " << "CameraMode::FREE" << std::endl;
    } else {
        std::cout << "cameraMode " << "CameraMode::FIXED" << std::endl;
        state.cameraMode = CameraMode::FIXED;
    }
}