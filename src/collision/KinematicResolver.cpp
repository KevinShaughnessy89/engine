#include "KinematicResolver.hpp"

#include "ResolvedConstraint.hpp"
#include "collision/shapes/Capsule.hpp"
#include "collision/shapes/Ray.hpp"
#include "components/collision/ShapeData.hpp"
#include "core/Core.hpp"
#include "environment/ChunkManager.hpp"
#include "environment/TriangleManager.hpp"
#include "rendering/CameraConstants.hpp"

ResolvedConstraint KinematicResolver::fromHeightMap(glm::vec3 position, const KinematicBody& body,
                                                    double deltaTime) {
    float height;
    if (Chunks.sampleBakedHeight(position.x, position.z, height)) {
        glm::vec3 finalPosition = position;
        finalPosition.y = height;
        return ResolvedConstraint{finalPosition, true};
    }
    // }
    return ResolvedConstraint{position, false};
}

ResolvedConstraint KinematicResolver::resolveCollisionConstraint(
    glm::vec3 position, const std::vector<uint32_t>& candidates, const KinematicBody& body,
    double deltaTime) {
    auto resolved = fromHeightMap(position, body, deltaTime);

    if (resolved.isGrounded) {
        return resolved;
    }

    return ResolvedConstraint{position, false};

    // auto tryGroundTriangle =
    //     [&](const TriangleData& triangleData) -> std::optional<ResolvedConstraint> {
    //     RayData rayData = RayData{};
    //     rayData.origin = position;
    //     rayData.origin.y += 50.f;
    //     rayData.direction = glm::vec3(0.f, -1.f, 0.f);

    //     auto result = Ray::intersectTriangle(rayData, triangleData);

    //     if (result.collided &&
    //         glm::length(position - result.contactPoint.position) <= groundTolerance) {
    //         glm::vec3 finalPosition = result.contactPoint.position;
    //         finalPosition.y += body.groundClearance -
    //                            body.groundClearance / 10.f;  // less than full head height to not
    //                                                     // separate so far that isGrounded ==
    //                                                     false
    //         return ResolvedConstraint{finalPosition, true};
    //     }

    //     return std::nullopt;
    // };

    // for (const auto& candidate : candidates) {
    //     entt::entity candidateEntity = static_cast<entt::entity>(candidate);

    //     if (Registry.valid(candidateEntity) && Registry.all_of<ShapeData>(candidateEntity)) {
    //         const ShapeData& shapeData = Registry.get<ShapeData>(candidateEntity);

    //         if (std::holds_alternative<TriangleData>(shapeData.data)) {
    //             if (auto resolved = tryGroundTriangle(std::get<TriangleData>(shapeData.data))) {
    //                 return *resolved;
    //             }
    //         } else if (std::holds_alternative<SphereData>(shapeData.data)) {
    //             // TODO: Ray vs Sphere grounding not yet implemented.
    //         } else if (std::holds_alternative<CapsuleData>(shapeData.data)) {
    //             const auto& capsuleData = std::get<CapsuleData>(shapeData.data);
    //             SphereData querySphere = body.collisionSphere;
    //             querySphere.currentCenter = position;
    //             auto result = Capsule::intersectWithSphere(capsuleData, querySphere);
    //             if (result.collided) {
    //                 glm::vec3 penetrationCorrect =
    //                     result.contactPoint.normal * result.contactPoint.penetrationDepth;
    //                 glm::vec3 finalPosition = position + penetrationCorrect;

    //                 return ResolvedConstraint{finalPosition, true};
    //             }
    //         } else if (std::holds_alternative<OBBData>(shapeData.data)) {
    //             // TODO: Ray vs OBB grounding not yet implemented.
    //         }
    //     }
    //     // Raw terrain triangle ids are no longer resolved here -- terrain grounding is handled
    //     // above via the chunk's baked heightmap.
    // }

    // return {position, false};
}

bool KinematicResolver::checkIsGrounded(const glm::vec3& normal) {
    return glm::dot(normal, glm::vec3(0, 1, 0)) > .7f;
}