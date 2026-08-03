#pragma once

#include "character/AnimationModelLoader.hpp"

struct SkeletonComponent {
    std::vector<BoneInfo> bones;
    std::vector<glm::mat4> animatedTransforms;
    int parentBoneID = -1;
    glm::vec3 pendingRootMotion{0.0f};
    glm::vec3 previousRootTranslation{0.0f};
    glm::vec3 previousRootDeltaPos{0.0f};
    int previousRootClipID = -1;
};