#pragma once
#include "core/PrecompiledHeader.hpp"
#include "fmod/api/core/inc/fmod.hpp"

class AudioController {
   private:
    FMOD::System* system;
    std::unordered_map<std::string, FMOD::Sound*> soundMap;

   public:
    AudioController();
    ~AudioController();

    void init();
    void update();
    void loadSound(const std::string& name, const std::string& filename, bool is3D = false,
                   bool isLooping = false);
    void playSound(const std::string& name);
    void stopSound(const std::string& name);
    void cleanup();
};
