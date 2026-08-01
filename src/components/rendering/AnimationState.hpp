#pragma once

#include <limits>

#include "AnimationClip.hpp"

struct AnimationState {
    int clipID;  // which AnimationClip this entity is currently playing
    int skinSlot =
        std::numeric_limits<int>::max();  // which slice of the shared bone SSBO this entity uses
    float currentTime = 0.0f;             // seconds into that clip — advanced every frame
    float playbackSpeed = 1.0f;           // multiplier on deltaTime, default 1.0
    bool looping = true;                  // whether to wrap or clamp at clip end
    int ssboBaseOffset = 0;               // this entity's slice of the shared bone SSBO
    std::vector<AnimationClip> clips;
    std::unordered_map<std::string, int> nameToID;
};