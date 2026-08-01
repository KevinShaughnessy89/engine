#include "KinematicController.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "collision/CollisionManager.hpp"
#include "collision/KinematicResolver.hpp"
#include "collision/ResolvedConstraint.hpp"
#include "collision/shapes/ColliderFactory.hpp"
#include "components/collision/ShapeData.hpp"
#include "components/physics/KinematicBody.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"

// Gravity, grounding, collision resolution, and the velocity/collider bookkeeping derived from
// the resolved position, for every KinematicBody in the registry. Facing (yaw) is written
// directly by MovementController, not synced from anywhere here.
void KinematicController::update(double deltaTime) {
    for (auto entity : Registry.view<KinematicBody>()) {
        updateBody(Registry.get<KinematicBody>(entity), deltaTime);
    }
}

void KinematicController::updateBody(KinematicBody& body, double deltaTime) {
    float stopThreshold = 0.01f;
    float lerpFactor = 150.f;

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
    glm::vec3 diff = resolvedConstraint.position - body.position;

    if (glm::dot(diff, diff) > stopThreshold * stopThreshold) {
        float alpha = 1.0f - std::exp(-lerpFactor * deltaTime);
        body.position += diff * alpha;
    } else {
        body.position = resolvedConstraint.position;
    }

    body.velocity = (body.position - body.previousPosition) / float(deltaTime);
    body.collisionSphere.currentCenter = body.position;
    if (body.previousPosition != body.position) body.previousPosition = body.position;
    body.deltaPos = glm::vec3(0.0f);
}

KinematicController::~KinematicController() {
}

KinematicController::KinematicController() {
}