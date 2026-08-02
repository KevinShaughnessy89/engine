#pragma once

#include <algorithm>

#include "components/rendering/AnimationClip.hpp"
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

inline void computeSkinMatrices(SkeletonComponent& skeleton, const AnimationClip& clip,
                                const AnimationState& animationState, bool clipLooped,
                                std::span<glm::mat4> skinMatrices) {
    const int numBones = skeleton.bones.size();

    std::vector<glm::mat4>& M_anim = skeleton.animatedTransforms;

    for (int i = 0; i < numBones; i++) {
        const BoneInfo& bone = skeleton.bones[i];

        // expensive, in the future map to channel indices instead of searching by name each frame
        auto channelIt = clip.channels.find(bone.name);
        if (channelIt != clip.channels.end()) {
            M_anim[i] = sampleChannel(channelIt->second, animationState.currentTime);
        } else {
            M_anim[i] = bone.localBindTransform;
        }
    }

    // Root motion: fold the root bone's clip-space translation into a world-space delta
    // (consumed by KinematicController::updateBody) instead of letting it move the mesh in
    // place, so the character's position, not just its skin, tracks the clip.
    const int rootID = skeleton.parentBoneID;
    if (rootID != -1) {
        glm::vec3 currentTranslation = glm::vec3(M_anim[rootID][3]);
        bool resync = clipLooped || skeleton.previousRootClipID != animationState.clipID;
        if (!resync) {
            skeleton.pendingRootMotion += currentTranslation - skeleton.previousRootTranslation;
        }
        skeleton.previousRootTranslation = currentTranslation;
        skeleton.previousRootClipID = animationState.clipID;

        // Zero the root bone's translation so it isn't baked into the skin a second time.
        M_anim[rootID][3] = glm::vec4(0.0f, skeleton.pendingRootMotion.y, 0.0f, 1.0f);
    }

    // skinMatrices doubles as scratch space for the global animated transforms (G_anim)
    // before being turned into the final skin matrices in the second pass below.
    for (int i = 0; i < numBones; i++) {
        const BoneInfo& bone = skeleton.bones[i];
        skinMatrices[i] = bone.parentID == -1 ? M_anim[i] : skinMatrices[bone.parentID] * M_anim[i];
    }

    for (int i = 0; i < numBones; i++) {
        skinMatrices[i] = skinMatrices[i] * skeleton.bones[i].offsetMatrix;
    }
}

inline void updateAnimationState(float deltaTime, GLuint ssbo) {
    auto view = Registry.view<AnimationState, SkeletonComponent>();

    std::vector<glm::mat4> allSkinMatrices(
        AnimationRenderer::MAX_BONES_PER_SKELETON * AnimationRenderer::MAX_ANIMATED_ENTITIES,
        glm::mat4(1.0f));

    int entityIndex = 0;
    for (auto entity : view) {
        auto& animationState = view.get<AnimationState>(entity);
        auto& skeleton = view.get<SkeletonComponent>(entity);

        std::vector<glm::mat4> skinMatrices(skeleton.bones.size());

        const AnimationClip& animationClip = animationState.clips[animationState.clipID];
        float rawTime = animationState.currentTime + deltaTime;
        bool clipLooped = rawTime >= animationClip.duration;
        animationState.currentTime = fmod(rawTime, animationClip.duration);

        animationState.skinSlot = entityIndex;

        glm::mat4* slice =
            allSkinMatrices.data() + entityIndex * AnimationRenderer::MAX_BONES_PER_SKELETON;
        std::span<glm::mat4> skinMatricesSpan(slice, skeleton.bones.size());
        computeSkinMatrices(skeleton, animationClip, animationState, clipLooped, skinMatricesSpan);

        entityIndex++;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, allSkinMatrices.size() * sizeof(glm::mat4),
                    allSkinMatrices.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
}  // namespace AnimationRenderer