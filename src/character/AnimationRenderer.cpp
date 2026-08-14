#include "character/AnimationRenderer.hpp"

#include <algorithm>

#include "IKFootCorrector.hpp"
#include "components/physics/KinematicBody.hpp"
#include "components/rendering/AnimationState.hpp"
#include "components/rendering/SkeletonComponent.hpp"
#include "rendering/UniformUpdater.hpp"

namespace AnimationRenderer {

int MAX_BONES_PER_SKELETON = 100;
int MAX_ANIMATED_ENTITIES = 64;

std::unordered_map<std::string, AnimationRegistry> registry;

}  // namespace AnimationRenderer

namespace {

struct AnimationBlendState {
    int fromClipID = -1;
    float fromTime = 0.0f;
    int toClipID = -1;
    float toTime = 0.0f;
    float blendElapsed = 0.0f;
    float blendDuration = 0.15f;
    bool isBlending = false;
};

void startClipTransition(AnimationBlendState& blendState, int nextClipID, float blendDuration) {
    blendState.fromClipID = blendState.toClipID != -1 ? blendState.toClipID : blendState.fromClipID;
    blendState.fromTime = blendState.toTime;
    blendState.toClipID = nextClipID;
    blendState.toTime = 0.0f;
    blendState.blendElapsed = 0.0f;
    blendState.blendDuration = blendDuration;
    blendState.isBlending = true;
}

glm::vec3 interpolatePosition(const std::vector<Vec3Key>& keys, float time) {
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

glm::quat interpolateRotation(const std::vector<QuatKey>& keys, float time) {
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

glm::vec3 interpolateScale(const std::vector<ScaleKey>& keys, float time) {
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

glm::mat4 sampleChannel(const AnimationChannel& channel, float time) {
    glm::vec3 pos = interpolatePosition(channel.positionKeys, time);  // lerp
    glm::quat rot = interpolateRotation(channel.rotationKeys, time);  // slerp
    glm::vec3 scale = interpolateScale(channel.scaleKeys, time);      // lerp

    glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
    glm::mat4 R = glm::mat4_cast(rot);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

    return T * R * S;
}

glm::mat4 sampleBlendedChannels(const AnimationBlendState& blendState,
                                const std::vector<AnimationClip>& clips, const std::string& boneName,
                                const glm::mat4& localBindTransform) {
    auto channelFor = [&](int clipID) -> const AnimationChannel* {
        if (clipID < 0 || clipID >= (int)clips.size()) return nullptr;
        auto channelIt = clips[clipID].channels.find(boneName);
        return channelIt != clips[clipID].channels.end() ? &channelIt->second : nullptr;
    };

    const AnimationChannel* from = channelFor(blendState.fromClipID);
    const AnimationChannel* to = channelFor(blendState.toClipID);
    if (!from && !to) return localBindTransform;
    if (!from) return sampleChannel(*to, blendState.toTime);
    if (!to) return sampleChannel(*from, blendState.fromTime);

    float factor = blendState.blendDuration > 0.0f
                       ? glm::clamp(blendState.blendElapsed / blendState.blendDuration, 0.0f, 1.0f)
                       : 1.0f;

    glm::vec3 position = glm::mix(interpolatePosition(from->positionKeys, blendState.fromTime),
                                  interpolatePosition(to->positionKeys, blendState.toTime), factor);
    glm::quat rotation = glm::slerp(interpolateRotation(from->rotationKeys, blendState.fromTime),
                                    interpolateRotation(to->rotationKeys, blendState.toTime), factor);
    glm::vec3 scale = glm::mix(interpolateScale(from->scaleKeys, blendState.fromTime),
                               interpolateScale(to->scaleKeys, blendState.toTime), factor);

    return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) *
           glm::scale(glm::mat4(1.0f), scale);
}

void computeSkinMatrices(SkeletonComponent& skeleton, const AnimationClip& clip,
                         const AnimationState& animationState, float rawTime,
                         std::span<glm::mat4> skinMatrices, const glm::mat4& modelMatrix,
                         bool isGrounded, float horizontalSpeed) {
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

    // Root motion: fold the root bone's clip-space XZ translation into a world-space delta
    // (consumed by KinematicController::updateBody) instead of letting it move the mesh in
    // place, so the character's position, not just its skin, tracks the clip. Y is left
    // untouched in the skin -- it's the rig's hip height/bob, not horizontal motion, and vertical
    // position is already owned by gravity/grounding in KinematicController.
    const int rootID = skeleton.parentBoneID;
    if (rootID != -1) {
        glm::vec3 currentTranslation = glm::vec3(M_anim[rootID][3]);

        bool clipChanged = skeleton.previousRootClipID != animationState.clipID;
        bool clipLooped = rawTime >= clip.duration;

        if (!clipLooped) {
            glm::vec3 delta = currentTranslation - skeleton.previousRootTranslation;
            glm::vec3 strippedDelta = glm::vec3(delta.x, 0.0f, delta.z);
            skeleton.pendingRootMotion += strippedDelta;
            skeleton.previousRootDeltaPos = strippedDelta;
        } else if (clipLooped && !clipChanged) {
            skeleton.pendingRootMotion += skeleton.previousRootDeltaPos;
        } else if (clipChanged) {
            skeleton.previousRootDeltaPos = glm::vec3(0.0f);
        }

        skeleton.previousRootTranslation = currentTranslation;
        skeleton.previousRootClipID = animationState.clipID;

        // Zero only the horizontal translation so it isn't baked into the skin a second time.
        M_anim[rootID][3].x = 0.0f;
        M_anim[rootID][3].z = 0.0f;
    }

    // skinMatrices doubles as scratch space for the global animated transforms (G_anim)
    // before being turned into the final skin matrices in the second pass below.
    int leftFootIdx = -1, rightFootIdx = -1, leftKneeIdx = -1, rightKneeIdx = -1, leftHipIdx = -1,
        rightHipIdx = -1;
    for (int i = 0; i < numBones; i++) {
        const BoneInfo& bone = skeleton.bones[i];
        if (bone.name == "LeftFoot") leftFootIdx = i;
        if (bone.name == "RightFoot") rightFootIdx = i;
        if (bone.name == "LeftLeg") leftKneeIdx = i;
        if (bone.name == "RightLeg") rightKneeIdx = i;
        if (bone.name == "LeftUpLeg") leftHipIdx = i;
        if (bone.name == "RightUpLeg") rightHipIdx = i;
        skinMatrices[i] = bone.parentID == -1 ? M_anim[i] : skinMatrices[bone.parentID] * M_anim[i];
    }

    IKFootCorrector::IKFootCorrect(skinMatrices, skeleton.bones, leftFootIdx, rightFootIdx,
                                   leftKneeIdx, rightKneeIdx, leftHipIdx, rightHipIdx, modelMatrix,
                                   isGrounded, horizontalSpeed);

    for (int i = 0; i < numBones; i++) {
        skinMatrices[i] = skinMatrices[i] * skeleton.bones[i].offsetMatrix;
    }
}

}  // namespace

namespace AnimationRenderer {

void updateAnimationState(float deltaTime, GLuint ssbo) {
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
        float rawTime = animationState.currentTime + deltaTime * animationState.playbackSpeed;
        animationState.currentTime = fmod(rawTime, animationClip.duration);

        animationState.skinSlot = entityIndex;

        glm::mat4* slice =
            allSkinMatrices.data() + entityIndex * AnimationRenderer::MAX_BONES_PER_SKELETON;
        std::span<glm::mat4> skinMatricesSpan(slice, skeleton.bones.size());
        glm::mat4 modelMatrix = UniformUpdater::calculateModelMatrix(entity);

        // Without a body there's no motion to read, so leave the solver on its penetration fallback.
        bool isGrounded = true;
        float horizontalSpeed = std::numeric_limits<float>::max();
        if (const auto* body = Registry.try_get<KinematicBody>(entity)) {
            isGrounded = body->isGrounded;
            horizontalSpeed = glm::length(glm::vec2(body->velocity.x, body->velocity.z));
        }

        computeSkinMatrices(skeleton, animationClip, animationState, rawTime, skinMatricesSpan,
                           modelMatrix, isGrounded, horizontalSpeed);

        entityIndex++;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, allSkinMatrices.size() * sizeof(glm::mat4),
                    allSkinMatrices.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

}  // namespace AnimationRenderer
