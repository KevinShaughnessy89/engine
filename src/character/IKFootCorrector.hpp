#pragma once
#include "core/Core.hpp"
#include "environment/ChunkManager.hpp"

namespace IKFootCorrector {

inline void solveLeg(glm::mat4& foot, glm::mat4& knee, glm::mat4& hip, const glm::vec3& target,
                     float thighLength, float shinLength) {
    glm::vec3 hipPos = glm::vec3(hip[3]);
    glm::vec3 kneePos = glm::vec3(knee[3]);
    glm::vec3 footPos = glm::vec3(foot[3]);

    glm::vec3 toTarget = target - hipPos;
    float rawDist = glm::length(toTarget);
    glm::vec3 targetDir = rawDist > 1e-5f ? toTarget / rawDist : glm::vec3(0.f, -1.f, 0.f);
    // Clamp to what the leg can physically reach, and clamp the foot target to match
    float targetDist = glm::min(rawDist, thighLength + shinLength - 0.001f);
    glm::vec3 reachableTarget = hipPos + targetDir * targetDist;

    float cosHipAngle =
        (thighLength * thighLength + targetDist * targetDist - shinLength * shinLength) /
        (2.f * thighLength * targetDist);
    float hipAngle = std::acos(glm::clamp(cosHipAngle, -1.f, 1.f));

    glm::vec3 hipToKnee = glm::normalize(kneePos - hipPos);
    // NOTE: sign of this axis determines which way the hip swings to satisfy hipAngle - verify
    // visually and flip (swap cross() argument order) if the knee bends the wrong direction.
    glm::vec3 bendAxis = glm::cross(targetDir, hipToKnee);
    float bendAxisLen = glm::length(bendAxis);
    bendAxis = bendAxisLen > 1e-5f ? bendAxis / bendAxisLen : glm::vec3(1.f, 0.f, 0.f);

    glm::vec3 newThighDir = glm::normalize(
        glm::vec3(glm::rotate(glm::mat4(1.f), hipAngle, bendAxis) * glm::vec4(targetDir, 0.f)));
    glm::vec3 newKneePos = hipPos + thighLength * newThighDir;
    glm::vec3 newShinDir = glm::normalize(reachableTarget - newKneePos);

    glm::vec3 hipDeltaAxis = glm::cross(hipToKnee, newThighDir);
    if (glm::length(hipDeltaAxis) > 1e-5f) {
        float hipDeltaAngle = std::acos(glm::clamp(glm::dot(hipToKnee, newThighDir), -1.f, 1.f));
        hip = glm::rotate(hip, hipDeltaAngle, glm::normalize(hipDeltaAxis));
    }

    glm::vec3 oldShinDir = glm::normalize(footPos - kneePos);
    glm::vec3 kneeDeltaAxis = glm::cross(oldShinDir, newShinDir);
    if (glm::length(kneeDeltaAxis) > 1e-5f) {
        float kneeDeltaAngle = std::acos(glm::clamp(glm::dot(oldShinDir, newShinDir), -1.f, 1.f));
        knee = glm::rotate(knee, kneeDeltaAngle, glm::normalize(kneeDeltaAxis));
    }

    knee[3] = glm::vec4(newKneePos, 1.f);
    foot[3] = glm::vec4(reachableTarget, 1.f);
}

// Tilts the foot's orientation to match the ground normal, without disturbing its position.
inline void alignFootToNormal(glm::mat4& foot, const glm::vec3& terrainNormal) {
    glm::vec3 currentUp = glm::normalize(glm::vec3(foot[1]));
    glm::vec3 rotationAxis = glm::cross(currentUp, terrainNormal);
    if (glm::length(rotationAxis) < 1e-5f) {
        return;
    }
    float angle = std::acos(glm::clamp(glm::dot(currentUp, terrainNormal), -1.f, 1.f));
    foot = glm::rotate(foot, angle, glm::normalize(rotationAxis));
}

inline void IKFootCorrect(glm::mat4& leftFoot, glm::mat4& rightFoot, glm::mat4& leftKnee,
                          glm::mat4& rightKnee, glm::mat4& leftHip, glm::mat4& rightHip,
                          glm::mat4& pelvis) {
    glm::vec3 leftFootTranslation = leftFoot[3];
    glm::vec3 rightFootTranslation = rightFoot[3];

    float leftFootTerrainHeight = 0.f;
    float rightFootTerrainHeight = 0.f;
    Chunks.sampleBakedHeight(leftFootTranslation.x, leftFootTranslation.z, leftFootTerrainHeight);
    Chunks.sampleBakedHeight(rightFootTranslation.x, rightFootTranslation.z,
                             rightFootTerrainHeight);

    glm::vec3 leftFootNormal =
        Chunks.sampleBakedNormal(leftFootTranslation.x, leftFootTranslation.z);
    glm::vec3 rightFootNormal =
        Chunks.sampleBakedNormal(rightFootTranslation.x, rightFootTranslation.z);

    // Bone lengths measured before any correction is applied, since they're invariant to it.
    float leftThighLength = glm::length(glm::vec3(leftKnee[3]) - glm::vec3(leftHip[3]));
    float leftShinLength = glm::length(leftFootTranslation - glm::vec3(leftKnee[3]));
    float rightThighLength = glm::length(glm::vec3(rightKnee[3]) - glm::vec3(rightHip[3]));
    float rightShinLength = glm::length(rightFootTranslation - glm::vec3(rightKnee[3]));

    float deltaLeftFoot = leftFootTerrainHeight - leftFootTranslation.y;
    float deltaRightFoot = rightFootTerrainHeight - rightFootTranslation.y;
    // Drop the pelvis by the worse (most negative) offset so neither leg has to overextend;
    // the leg with the smaller offset just ends up bent rather than fully straight.
    float pelvisOffset = std::min(deltaLeftFoot, deltaRightFoot);

    pelvis[3].y += pelvisOffset;
    leftHip[3].y += pelvisOffset;
    rightHip[3].y += pelvisOffset;

    glm::vec3 leftTarget(leftFootTranslation.x, leftFootTerrainHeight, leftFootTranslation.z);
    glm::vec3 rightTarget(rightFootTranslation.x, rightFootTerrainHeight, rightFootTranslation.z);

    solveLeg(leftFoot, leftKnee, leftHip, leftTarget, leftThighLength, leftShinLength);
    solveLeg(rightFoot, rightKnee, rightHip, rightTarget, rightThighLength, rightShinLength);

    alignFootToNormal(leftFoot, leftFootNormal);
    alignFootToNormal(rightFoot, rightFootNormal);
}

}  // namespace IKFootCorrector
