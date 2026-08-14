#pragma once
#include <span>

#include "character/AnimationModelLoader.hpp"
#include "core/Core.hpp"
#include "environment/ChunkManager.hpp"

namespace IKFootCorrector {

inline const glm::vec3 fallbackKneeForward(0.f, 0.f, 1.f);
// Speeds, in leg lengths per second, bounding the fade from a fully planted stance down to
// penetration-only correction. Idle shifting must stay under the first, locomotion over the second.
inline const float stationarySpeed = 0.5f;
inline const float locomotionSpeed = 1.5f;

struct LegChain {
    int hipID = -1;
    int kneeID = -1;
    int footID = -1;
    float thighLength = 0.f;
    float shinLength = 0.f;
    float ankleHeight = 0.f;
    glm::vec3 localHingeAxis{0.f};
};

inline glm::vec3 perpendicularComponent(const glm::vec3& v, const glm::vec3& unitAxis) {
    return v - glm::dot(v, unitAxis) * unitAxis;
}

inline glm::vec3 anyPerpendicular(const glm::vec3& unitAxis) {
    glm::vec3 seed =
        std::abs(unitAxis.x) < 0.9f ? glm::vec3(1.f, 0.f, 0.f) : glm::vec3(0.f, 1.f, 0.f);
    return glm::normalize(glm::cross(unitAxis, seed));
}

inline bool isInSubtree(const std::vector<BoneInfo>& bones, int boneID, int ancestorID) {
    for (int id = boneID; id != -1; id = bones[id].parentID) {
        if (id == ancestorID) return true;
    }
    return false;
}

inline void rotateSubtree(std::span<glm::mat4> skinMatrices, const std::vector<BoneInfo>& bones,
                          int rootBoneID, const glm::mat3& rotation, const glm::vec3& pivot) {
    for (int i = 0; i < (int)bones.size(); i++) {
        if (!isInSubtree(bones, i, rootBoneID)) continue;
        glm::mat4& bone = skinMatrices[i];
        glm::vec3 pivotOffset = glm::vec3(bone[3]) - pivot;
        bone = glm::mat4(rotation * glm::mat3(bone));
        bone[3] = glm::vec4(pivot + rotation * pivotOffset, 1.f);
    }
}

inline glm::mat3 orthonormalBasis(const glm::vec3& primary, const glm::vec3& secondary) {
    glm::vec3 x = glm::normalize(primary);
    glm::vec3 y = perpendicularComponent(secondary, x);
    y = glm::length(y) > 1e-5f ? glm::normalize(y) : anyPerpendicular(x);
    return glm::mat3(x, y, glm::cross(x, y));
}

inline glm::mat4 bindGlobalTransform(const std::vector<BoneInfo>& bones, int boneID) {
    glm::mat4 transform(1.f);
    for (int id = boneID; id != -1; id = bones[id].parentID) {
        transform = bones[id].localBindTransform * transform;
    }
    return transform;
}

inline int firstChildBone(const std::vector<BoneInfo>& bones, int parentID) {
    for (int i = 0; i < (int)bones.size(); i++) {
        if (bones[i].parentID == parentID) return i;
    }
    return -1;
}

// From the toes, not the bind knee offset - Mixamo T-pose knees are hyperextended backwards.
inline glm::vec3 bindKneeForwardDirection(const std::vector<BoneInfo>& bones, int hipID,
                                          int footID) {
    glm::vec3 bindHip = glm::vec3(bindGlobalTransform(bones, hipID)[3]);
    glm::vec3 bindFoot = glm::vec3(bindGlobalTransform(bones, footID)[3]);
    glm::vec3 legAxis = bindFoot - bindHip;
    float legLength = glm::length(legAxis);
    int toeID = firstChildBone(bones, footID);
    if (legLength < 1e-5f || toeID == -1) return fallbackKneeForward;

    glm::vec3 bindToe = glm::vec3(bindGlobalTransform(bones, toeID)[3]);
    glm::vec3 forward = perpendicularComponent(bindToe - bindFoot, legAxis / legLength);
    return glm::length(forward) > 1e-3f * legLength ? glm::normalize(forward) : fallbackKneeForward;
}

// Hinge from the knee's own frame; the toes only pick between axes, they splay 19 degrees out.
inline glm::vec3 localKneeHingeAxis(const std::vector<BoneInfo>& bones, int kneeID, int footID,
                                    const glm::vec3& toeForward) {
    glm::mat4 bindKnee = bindGlobalTransform(bones, kneeID);
    glm::mat3 bindRotation = glm::mat3(bindKnee);
    glm::vec3 bindFoot = glm::vec3(bindGlobalTransform(bones, footID)[3]);
    glm::vec3 boneDirection = bindFoot - glm::vec3(bindKnee[3]);
    if (glm::length(boneDirection) < 1e-5f) return glm::vec3(1.f, 0.f, 0.f);
    boneDirection = glm::normalize(boneDirection);

    int boneAxis = 0;
    float strongestAlignment = -1.f;
    for (int i = 0; i < 3; i++) {
        float alignment = std::abs(glm::dot(glm::normalize(bindRotation[i]), boneDirection));
        if (alignment > strongestAlignment) {
            strongestAlignment = alignment;
            boneAxis = i;
        }
    }

    int hingeAxis = -1;
    float weakestForward = 2.f;
    for (int i = 0; i < 3; i++) {
        if (i == boneAxis) continue;
        float alignment = std::abs(glm::dot(glm::normalize(bindRotation[i]), toeForward));
        if (alignment < weakestForward) {
            weakestForward = alignment;
            hingeAxis = i;
        }
    }

    glm::vec3 localAxis(0.f);
    localAxis[hingeAxis] = 1.f;
    glm::vec3 bindHinge = glm::normalize(bindRotation * localAxis);
    if (glm::dot(glm::cross(bindHinge, boneDirection), toeForward) < 0.f) {
        localAxis[hingeAxis] = -1.f;
    }
    return localAxis;
}

inline float bindAnkleHeight(const std::vector<BoneInfo>& bones, int footID) {
    float ankleY = bindGlobalTransform(bones, footID)[3].y;
    float lowestY = ankleY;
    for (int i = 0; i < (int)bones.size(); i++) {
        if (!isInSubtree(bones, i, footID)) continue;
        lowestY = glm::min(lowestY, bindGlobalTransform(bones, i)[3].y);
    }
    return ankleY - lowestY;
}

inline LegChain buildLegChain(const std::vector<BoneInfo>& bones,
                              std::span<const glm::mat4> skinMatrices, int hipID, int kneeID,
                              int footID) {
    LegChain leg;
    leg.hipID = hipID;
    leg.kneeID = kneeID;
    leg.footID = footID;

    glm::vec3 hipPos = glm::vec3(skinMatrices[hipID][3]);
    glm::vec3 kneePos = glm::vec3(skinMatrices[kneeID][3]);
    glm::vec3 footPos = glm::vec3(skinMatrices[footID][3]);
    leg.thighLength = glm::length(kneePos - hipPos);
    leg.shinLength = glm::length(footPos - kneePos);
    leg.ankleHeight = bindAnkleHeight(bones, footID);
    leg.localHingeAxis =
        localKneeHingeAxis(bones, kneeID, footID, bindKneeForwardDirection(bones, hipID, footID));
    return leg;
}

inline void solveLeg(std::span<glm::mat4> skinMatrices, const std::vector<BoneInfo>& bones,
                     const LegChain& leg, const glm::vec3& target) {
    if (leg.thighLength < 1e-5f || leg.shinLength < 1e-5f) return;

    glm::vec3 hipPos = glm::vec3(skinMatrices[leg.hipID][3]);
    glm::vec3 kneePos = glm::vec3(skinMatrices[leg.kneeID][3]);

    glm::vec3 toTarget = target - hipPos;
    float rawDistance = glm::length(toTarget);
    glm::vec3 targetDir = rawDistance > 1e-5f ? toTarget / rawDistance : glm::vec3(0.f, -1.f, 0.f);

    float minReach = std::abs(leg.thighLength - leg.shinLength) + 1e-4f;
    float maxReach = glm::max(leg.thighLength + leg.shinLength - 1e-3f, minReach);
    float targetDist = glm::clamp(rawDistance, minReach, maxReach);
    glm::vec3 reachableTarget = hipPos + targetDir * targetDist;

    float cosHipAngle = (leg.thighLength * leg.thighLength + targetDist * targetDist -
                         leg.shinLength * leg.shinLength) /
                        (2.f * leg.thighLength * targetDist);
    float hipAngle = std::acos(glm::clamp(cosHipAngle, -1.f, 1.f));

    glm::vec3 hinge = glm::normalize(glm::mat3(skinMatrices[leg.kneeID]) * leg.localHingeAxis);
    glm::vec3 pole = glm::cross(hinge, targetDir);
    float poleLength = glm::length(pole);
    if (poleLength < 1e-4f) return;
    pole /= poleLength;

    glm::vec3 newThighDir = std::cos(hipAngle) * targetDir + std::sin(hipAngle) * pole;
    glm::vec3 newKneePos = hipPos + leg.thighLength * newThighDir;
    glm::vec3 newShinDir = glm::normalize(reachableTarget - newKneePos);

    glm::vec3 bendNormal = glm::normalize(glm::cross(targetDir, pole));
    glm::vec3 oldThighDir = glm::normalize(kneePos - hipPos);
    glm::mat3 hipRotation = orthonormalBasis(newThighDir, bendNormal) *
                            glm::transpose(orthonormalBasis(oldThighDir, hinge));
    rotateSubtree(skinMatrices, bones, leg.hipID, hipRotation, hipPos);

    glm::vec3 hingeAfterHip =
        glm::normalize(glm::mat3(skinMatrices[leg.kneeID]) * leg.localHingeAxis);
    glm::vec3 shinDirAfterHip = glm::normalize(glm::vec3(skinMatrices[leg.footID][3]) -
                                               glm::vec3(skinMatrices[leg.kneeID][3]));
    float kneeAngle = std::atan2(glm::dot(glm::cross(shinDirAfterHip, newShinDir), hingeAfterHip),
                                 glm::dot(shinDirAfterHip, newShinDir));
    rotateSubtree(skinMatrices, bones, leg.kneeID,
                  glm::mat3(glm::rotate(glm::mat4(1.f), kneeAngle, hingeAfterHip)), newKneePos);
}

inline void alignFootToNormal(std::span<glm::mat4> skinMatrices, const std::vector<BoneInfo>& bones,
                              int footID, const glm::vec3& terrainNormal, float weight) {
    glm::vec3 localUp(0.f, 1.f, 0.f);
    float slope = weight * std::acos(glm::clamp(glm::dot(localUp, terrainNormal), -1.f, 1.f));
    if (slope < 1e-5f) return;

    glm::vec3 axis = glm::cross(localUp, terrainNormal);
    float axisLength = glm::length(axis);
    if (axisLength < 1e-6f) return;

    glm::mat3 tilt = glm::mat3(glm::rotate(glm::mat4(1.f), slope, axis / axisLength));
    rotateSubtree(skinMatrices, bones, footID, tilt, glm::vec3(skinMatrices[footID][3]));
}

inline void IKFootCorrect(std::span<glm::mat4> skinMatrices, const std::vector<BoneInfo>& bones,
                          int leftFootIdx, int rightFootIdx, int leftKneeIdx, int rightKneeIdx,
                          int leftHipIdx, int rightHipIdx, const glm::mat4& modelMatrix,
                          bool isGrounded, float horizontalSpeed) {
    if (leftFootIdx < 0 || rightFootIdx < 0 || leftKneeIdx < 0 || rightKneeIdx < 0 ||
        leftHipIdx < 0 || rightHipIdx < 0) {
        return;
    }
    if (!isGrounded) return;

    LegChain leftLeg = buildLegChain(bones, skinMatrices, leftHipIdx, leftKneeIdx, leftFootIdx);
    LegChain rightLeg = buildLegChain(bones, skinMatrices, rightHipIdx, rightKneeIdx, rightFootIdx);

    // A standing pose is the one the animator posed deliberately, so hold both feet to the ground;
    // once the character is moving, only penetration justifies touching a foot.
    float modelScale = glm::length(glm::vec3(modelMatrix[0]));
    float legLength = modelScale * glm::min(leftLeg.thighLength + leftLeg.shinLength,
                                            rightLeg.thighLength + rightLeg.shinLength);
    float legLengthsPerSecond = legLength > 1e-5f ? horizontalSpeed / legLength : locomotionSpeed;
    float stationaryWeight =
        1.f - glm::smoothstep(stationarySpeed, locomotionSpeed, legLengthsPerSecond);

    glm::mat4 invModel = glm::inverse(modelMatrix);
    glm::mat3 invModelRotation = glm::mat3(invModel);

    glm::vec3 leftFootLocal = glm::vec3(skinMatrices[leftFootIdx][3]);
    glm::vec3 rightFootLocal = glm::vec3(skinMatrices[rightFootIdx][3]);
    glm::vec3 leftFootWorld = glm::vec3(modelMatrix * glm::vec4(leftFootLocal, 1.f));
    glm::vec3 rightFootWorld = glm::vec3(modelMatrix * glm::vec4(rightFootLocal, 1.f));

    float leftTerrainHeightWorld = 0.f;
    float rightTerrainHeightWorld = 0.f;
    bool leftSampled =
        Chunks.sampleBakedHeight(leftFootWorld.x, leftFootWorld.z, leftTerrainHeightWorld);
    bool rightSampled =
        Chunks.sampleBakedHeight(rightFootWorld.x, rightFootWorld.z, rightTerrainHeightWorld);
    if (!leftSampled || !rightSampled) {
        return;
    }

    glm::vec3 leftNormalWorld = Chunks.sampleBakedNormal(leftFootWorld.x, leftFootWorld.z);
    glm::vec3 rightNormalWorld = Chunks.sampleBakedNormal(rightFootWorld.x, rightFootWorld.z);
    glm::vec3 leftNormalLocal = glm::normalize(invModelRotation * leftNormalWorld);
    glm::vec3 rightNormalLocal = glm::normalize(invModelRotation * rightNormalWorld);

    float leftAnkleTargetWorld = leftTerrainHeightWorld + leftLeg.ankleHeight;
    float rightAnkleTargetWorld = rightTerrainHeightWorld + rightLeg.ankleHeight;

    // Positive means the ankle sits below its terrain-derived target, i.e. the foot has penetrated.
    float leftPenetration = leftAnkleTargetWorld - leftFootWorld.y;
    float rightPenetration = rightAnkleTargetWorld - rightFootWorld.y;
    // Penetration is a hard constraint, so it overrides the speed fade; a floating foot only gets
    // pulled down while the character is standing still.
    float leftWeight = leftPenetration > 0.f ? 1.f : stationaryWeight;
    float rightWeight = rightPenetration > 0.f ? 1.f : stationaryWeight;
    if (leftWeight <= 0.f && rightWeight <= 0.f) {
        return;
    }

    // Tilt needs its own weight ramped over ankle depth -- it scales an angle rather than a
    // vanishing offset, so reusing the step above would snap the foot as it crosses the surface.
    float ankleDepth =
        glm::max(modelScale * glm::min(leftLeg.ankleHeight, rightLeg.ankleHeight), 1e-5f);
    float leftTiltWeight =
        glm::max(stationaryWeight, glm::clamp(leftPenetration / ankleDepth, 0.f, 1.f));
    float rightTiltWeight =
        glm::max(stationaryWeight, glm::clamp(rightPenetration / ankleDepth, 0.f, 1.f));

    // Partial weight blends the target back toward where the animation already put the foot.
    float leftTargetY = leftFootWorld.y + leftWeight * leftPenetration;
    float rightTargetY = rightFootWorld.y + rightWeight * rightPenetration;
    glm::vec3 leftTargetWorld(leftFootWorld.x, leftTargetY, leftFootWorld.z);
    glm::vec3 rightTargetWorld(rightFootWorld.x, rightTargetY, rightFootWorld.z);
    glm::vec3 leftTargetLocal = glm::vec3(invModel * glm::vec4(leftTargetWorld, 1.f));
    glm::vec3 rightTargetLocal = glm::vec3(invModel * glm::vec4(rightTargetWorld, 1.f));

    // The pelvis absorbs the shallowest correction; the legs bend off whatever difference is left.
    float leftOffset = leftWeight * leftPenetration;
    float rightOffset = rightWeight * rightPenetration;
    float pelvisOffset = leftWeight > 0.f && rightWeight > 0.f
                             ? std::min(leftOffset, rightOffset)
                             : (leftWeight > 0.f ? leftOffset : rightOffset);

    auto reachInterval = [&](const LegChain& leg, const glm::vec3& targetLocal, float& low,
                             float& high) {
        glm::vec3 toTarget = targetLocal - glm::vec3(skinMatrices[leg.hipID][3]);
        float reach = leg.thighLength + leg.shinLength - 1e-3f;
        float horizontal = glm::length(glm::vec2(toTarget.x, toTarget.z));
        if (horizontal >= reach) return false;
        float span = std::sqrt(reach * reach - horizontal * horizontal);
        low = toTarget.y - span;
        high = toTarget.y + span;
        return true;
    };

    float lowestAllowed = std::numeric_limits<float>::lowest();
    float highestAllowed = std::numeric_limits<float>::max();
    float low = 0.f;
    float high = 0.f;
    if (leftWeight > 0.f && reachInterval(leftLeg, leftTargetLocal, low, high)) {
        lowestAllowed = glm::max(lowestAllowed, low);
        highestAllowed = glm::min(highestAllowed, high);
    }
    if (rightWeight > 0.f && reachInterval(rightLeg, rightTargetLocal, low, high)) {
        lowestAllowed = glm::max(lowestAllowed, low);
        highestAllowed = glm::min(highestAllowed, high);
    }
    if (lowestAllowed > highestAllowed) {
        float compromise = 0.5f * (lowestAllowed + highestAllowed);
        lowestAllowed = compromise;
        highestAllowed = compromise;
    }
    pelvisOffset = glm::clamp(pelvisOffset, lowestAllowed, highestAllowed);

    for (glm::mat4& boneMatrix : skinMatrices) {
        boneMatrix[3].y += pelvisOffset;
    }

    if (leftWeight > 0.f) {
        solveLeg(skinMatrices, bones, leftLeg, leftTargetLocal);
        alignFootToNormal(skinMatrices, bones, leftFootIdx, leftNormalLocal, leftTiltWeight);
    }
    if (rightWeight > 0.f) {
        solveLeg(skinMatrices, bones, rightLeg, rightTargetLocal);
        alignFootToNormal(skinMatrices, bones, rightFootIdx, rightNormalLocal, rightTiltWeight);
    }
}

}  // namespace IKFootCorrector
