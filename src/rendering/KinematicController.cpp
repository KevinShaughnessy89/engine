#include "KinematicController.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "collision/CollisionManager.hpp"
#include "collision/KinematicResolver.hpp"
#include "collision/ResolvedConstraint.hpp"
#include "components/collision/ShapeData.hpp"
#include "components/physics/KinematicBody.hpp"
#include "components/rendering/CameraState.hpp"
#include "components/rendering/Renderable.hpp"
#include "components/rendering/SkeletonComponent.hpp"
#include "components/rendering/ViewState.hpp"
#include "core/Core.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "rendering/CameraConstants.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/CameraMode.hpp"
// Gravity, grounding, collision resolution, and the velocity/collider bookkeeping derived from
// the resolved position, for every KinematicBody in the registry. Facing (yaw) is written
// directly by MovementController, not synced from anywhere here.
void KinematicController::update(double deltaTime) {
    for (auto entity : Registry.view<KinematicBody>()) {
        updateBody(entity, deltaTime);
    }
}

void KinematicController::updateBody(entt::entity entity, double deltaTime) {
    auto& body = Registry.get<KinematicBody>(entity);
    auto& skeleton = Registry.get<SkeletonComponent>(entity);

    auto& view = Registry.get<ViewState>(CameraController::activeCamera);
    auto& cameraState = Registry.get<CameraState>(CameraController::activeCamera);

    // Fold this frame's root motion (local-space, accumulated by AnimationRenderer) into deltaPos
    // as a world-space offset, same contract MovementController uses for input-driven movement.
    glm::mat4 R = glm::mat4_cast(body.orientation);
    if (auto* renderable = Registry.try_get<Renderable>(entity)) {
        R = R * glm::rotate(glm::mat4(1.0f), glm::radians(renderable->forwardOffsetDegrees),
                            CameraConstants::WORLD_UP_AXIS);
    }
    body.deltaPos += glm::vec3(R * glm::vec4(skeleton.pendingRootMotion, 0.0f));
    skeleton.pendingRootMotion = glm::vec3(0.0f);

    if (!body.isGrounded) {
        body.wasGrounded = false;
        body.verticalVelocity += CameraConstants::gravity * (deltaTime / 10);
        body.verticalVelocity =
            std::clamp(body.verticalVelocity, CameraConstants::terminalVelocity, 0.0f);
        body.targetPosition.y = body.position.y + body.verticalVelocity * (deltaTime / 10);
    }

    if (body.isGrounded && body.wasGrounded) {
        body.targetPosition = body.position + body.deltaPos;
    }

    if (body.isGrounded && !body.wasGrounded) {
        body.verticalVelocity = 0.0f;
        body.targetPosition = body.position;
        body.wasGrounded = true;
    }

    CandidateSet candidates = Collision.getCandidates(body.targetPosition);

    std::vector<uint32_t> combinedCandidates;
    for (auto entity : candidates.dynamicCandidates) {
        combinedCandidates.push_back(static_cast<uint32_t>(entity));
    }
    combinedCandidates.insert(combinedCandidates.end(), candidates.triangleCandidates.begin(),
                              candidates.triangleCandidates.end());

    ResolvedConstraint resolvedConstraint = KinematicResolver::resolveCollisionConstraint(
        body.targetPosition, combinedCandidates, body, deltaTime);

    body.isGrounded = resolvedConstraint.isGrounded;
    body.position = resolvedConstraint.position;

    body.velocity = (body.position - body.previousPosition) / float(deltaTime);
    body.collisionSphere.currentCenter = body.position;
    if (cameraState.cameraMode == CameraMode::FIXED) {
        body.orientation =
            glm::quatLookAt(glm::normalize(glm::vec3(view.forward.x, 0.0f, view.forward.z)),
                            CameraConstants::WORLD_UP_AXIS);
    }
    if (body.previousPosition != body.position) body.previousPosition = body.position;
    body.deltaPos = glm::vec3(0.0f);
}

KinematicController::~KinematicController() {
}

KinematicController::KinematicController() {
}