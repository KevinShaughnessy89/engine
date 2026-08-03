#pragma once

#include "components/rendering/AnimationClip.hpp"
#include "core/Core.hpp"
#include "entt-main/src/entt/entt.hpp"

struct AnimationBlendState;
struct AnimationRegistry {
    std::vector<AnimationClip> clips;
    std::unordered_map<std::string, int> nameToID;
};

namespace AnimationRenderer {

extern int MAX_BONES_PER_SKELETON;
extern int MAX_ANIMATED_ENTITIES;

extern std::unordered_map<std::string, AnimationRegistry> registry;

void updateAnimationState(float deltaTime, GLuint ssbo);
void startClipTransition(AnimationBlendState& blendState, int nextClipID, float blendDuration);

}  // namespace AnimationRenderer
