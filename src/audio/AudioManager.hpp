#pragma once
#include "core/PrecompiledHeader.hpp"


#include "fmod/api/core/inc/fmod.hpp"

class AudioManager {
private:
    FMOD::System* system;
    std::unordered_map<std::string, FMOD::Sound*> soundMap;

public:
    AudioManager();
    ~AudioManager();

    bool init();
    void update();
    bool loadSound(const std::string& name, const std::string& filename, bool is3D = false, bool isLooping = false);
    void playSound(const std::string& name);
    void stopSound(const std::string& name);
    void cleanup();
};
