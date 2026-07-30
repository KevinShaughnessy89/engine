#pragma once

#include <algorithm>

#include "components/rendering/AnimationClip.hpp"
#include "components/rendering/AnimationRegistryComponent.hpp"
#include "components/rendering/AnimationState.hpp"
#include "components/rendering/SkeletonComponent.hpp"
#include "core/Core.hpp"
#include "entt-main/src/entt/entt.hpp"

struct AnimationRegistry {
    std::vector<AnimationClip> clips;
    std::unordered_map<std::string, int> nameToID;
};

namespace AnimationRenderer {

inline int MAX_BONES_PER_SKELETON = 100;
inline int MAX_ANIMATED_ENTITIES = 64;

inline std::unordered_map<std::string, AnimationRegistry> registry;

inline glm::vec3 interpolatePosition(const std::vector<Vec3Key>& keys, float time) {
    if (keys.size() == 1) return keys[0].value;

    int nextIndex = 0;
    for (int i = 0; i < (int)keys.size(); i++) {
        if (time < keys[i].time) {
            nextIndex = i;
            break;
        }
        nextIndex = (int)keys.size() - 1;
    }

    int prevIndex = std::max(0, nextIndex - 1);

    if (prevIndex == nextIndex)
        return keys[prevIndex].value;  // t before first key, or only one usable key

    float t0 = keys[prevIndex].time;
    float t1 = keys[nextIndex].time;

    float factor = (time - t0) / (t1 - t0);
    factor = std::clamp(factor, 0.0f, 1.0f);
    return glm::mix(keys[prevIndex].value, keys[nextIndex].value, factor);
}

inline glm::quat interpolateRotation(const std::vector<QuatKey>& keys, float time) {
    if (keys.size() == 1) return keys[0].value;

    int nextIndex = 0;
    for (int i = 0; i < (int)keys.size(); i++) {
        if (time < keys[i].time) {
            nextIndex = i;
            break;
        }
        nextIndex = (int)keys.size() - 1;
    }
    int prevIndex = std::max(nextIndex - 1, 0);

    if (prevIndex == nextIndex) return keys[prevIndex].value;

    float t0 = keys[prevIndex].time;
    float t1 = keys[nextIndex].time;
    float factor = glm::clamp((time - t0) / (t1 - t0), 0.0f, 1.0f);

    return glm::slerp(keys[prevIndex].value, keys[nextIndex].value, factor);
}

inline glm::vec3 interpolateScale(const std::vector<ScaleKey>& keys, float time) {
    if (keys.size() == 1) return keys[0].value;

    int nextIndex = 0;
    for (int i = 0; i < (int)keys.size(); i++) {
        if (time < keys[i].time) {
            nextIndex = i;
            break;
        }
        nextIndex = (int)keys.size() - 1;
    }
    int prevIndex = std::max(nextIndex - 1, 0);

    if (prevIndex == nextIndex) return keys[prevIndex].value;

    float t0 = keys[prevIndex].time;
    float t1 = keys[nextIndex].time;
    float factor = glm::clamp((time - t0) / (t1 - t0), 0.0f, 1.0f);

    return glm::mix(keys[prevIndex].value, keys[nextIndex].value, factor);
}

inline glm::mat4 sampleChannel(const AnimationChannel& channel, float time) {
    glm::vec3 pos = interpolatePosition(channel.positionKeys, time);  // lerp
    glm::quat rot = interpolateRotation(channel.rotationKeys, time);  // slerp
    glm::vec3 scale = interpolateScale(channel.scaleKeys, time);      // lerp

    glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
    glm::mat4 R = glm::mat4_cast(rot);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

    return T * R * S;
}

inline void computeSkinMatrices(const SkeletonComponent& skeleton, const AnimationClip& clip,
                                const AnimationState& animationState,
                                std::span<glm::mat4> skinMatrices) {
    const int numBones = skeleton.bones.size();

    // Scratch space for this entity's global animated transforms.
    // Reuse a thread_local or pre-sized member buffer in real code to avoid
    // a per-entity per-frame heap allocation.
    std::vector<glm::mat4> G_anim(numBones);

    for (int i = 0; i < numBones; i++) {
        const BoneInfo& bone = skeleton.bones[i];

        glm::mat4 M_anim;
        // expensive, in the future map to channel indices instead of searching by name each frame
        auto channelIt = clip.channels.find(bone.name);
        if (channelIt != clip.channels.end()) {
            M_anim = sampleChannel(channelIt->second, animationState.currentTime);
        } else {
            M_anim = bone.localBindTransform;
        }

        if (bone.parentID == -1) {
            G_anim[i] = M_anim;
        } else {
            G_anim[i] = G_anim[bone.parentID] * M_anim;
        }
    }

    for (int i = 0; i < numBones; i++) {
        skinMatrices[i] = G_anim[i] * skeleton.bones[i].offsetMatrix;
    }
}

inline void updateAnimationState(float deltaTime, GLuint ssbo) {
    auto view = Registry.view<AnimationState, SkeletonComponent, AnimationRegistryComponent>();

    std::vector<glm::mat4> allSkinMatrices(
        AnimationRenderer::MAX_BONES_PER_SKELETON * AnimationRenderer::MAX_ANIMATED_ENTITIES,
        glm::mat4(1.0f));

    int entityIndex = 0;
    for (auto entity : view) {
        auto& animationState = view.get<AnimationState>(entity);
        auto& skeleton = view.get<SkeletonComponent>(entity);
        auto& registryComponent = view.get<AnimationRegistryComponent>(entity);

        std::vector<glm::mat4> skinMatrices(skeleton.bones.size());

        const AnimationClip& animationClip =
            registryComponent.registry->clips[animationState.clipID];
        animationState.currentTime += deltaTime * animationClip.ticksPerSecond;
        animationState.currentTime = fmod(animationState.currentTime, animationClip.duration);

        animationState.skinSlot = entityIndex;

        glm::mat4* slice =
            allSkinMatrices.data() + entityIndex * AnimationRenderer::MAX_BONES_PER_SKELETON;
        std::span<glm::mat4> skinMatricesSpan(slice, skeleton.bones.size());
        computeSkinMatrices(skeleton, animationClip, animationState, skinMatricesSpan);

        entityIndex++;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, allSkinMatrices.size() * sizeof(glm::mat4),
                    allSkinMatrices.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
}  // namespace AnimationRenderer