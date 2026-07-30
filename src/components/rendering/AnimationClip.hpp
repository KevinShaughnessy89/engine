#pragma once

struct Vec3Key {
    float time;
    glm::vec3 value;
};

struct QuatKey {
    float time;
    glm::quat value;
};

struct ScaleKey {
    float time;
    glm::vec3 value;
};

struct AnimationChannel {
    std::string boneName;  // matches skeleton bone by name
    std::vector<Vec3Key> positionKeys;
    std::vector<QuatKey> rotationKeys;
    std::vector<ScaleKey> scaleKeys;
};

struct AnimationClip {
    std::string name;  // "Walk", "Idle", etc
    float duration;    // in seconds (convert from ticks)
    float ticksPerSecond;
    std::unordered_map<std::string, AnimationChannel> channels;  // keyed by bone name
};