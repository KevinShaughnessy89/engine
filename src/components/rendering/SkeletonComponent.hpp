#pragma once

#include "character/AnimationModelLoader.hpp"

struct SkeletonComponent {
    std::vector<BoneInfo> bones;
    std::vector<glm::mat4> animatedTransforms;
    int parentBoneID = -1;
    // Local-space translation delta of the root bone accumulated this render frame by
    // AnimationRenderer::computeSkinMatrices; consumed and reset by KinematicController::updateBody.
    glm::vec3 pendingRootMotion{0.0f};
    // Root-motion delta bookkeeping, updated each frame in AnimationRenderer::computeSkinMatrices.
    glm::vec3 previousRootTranslation{0.0f};
    int previousRootClipID = -1;  // -1 forces a resync (no delta contributed) on the first sample
};