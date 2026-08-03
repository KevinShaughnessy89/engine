#pragma once

#include "fmod/api/core/inc/fmod.hpp"

namespace AudioController {

struct Sound {
    std::string name;
    FMOD::Sound* soundPtr = nullptr;
    bool is3D = false;
    bool isLooping = false;
    float loopFrequency = 0.0f;  // 0 -> not periodic
    float timeSinceLastPlay = 0.0f;
    std::vector<FMOD::Channel*> activeChannels;
};

inline std::unordered_map<std::string, Sound> soundMap;
inline FMOD::System* system;

void init();
void update(double deltaTime);
void loadSound(const std::string& name, const std::string& filename, bool is3D = false,
               bool isLooping = false);
void playSound(const std::string& name);
void stopSound(const std::string& name);
void cleanup();
};  // namespace AudioController
