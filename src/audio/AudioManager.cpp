#include "core/PrecompiledHeader.hpp"
#include "AudioManager.hpp"

AudioManager::AudioManager() : system(nullptr) {

}

AudioManager::~AudioManager() {
    cleanup();
}

bool AudioManager::init() {

    FMOD_RESULT result = FMOD::System_Create(&system);
    
    if (result != FMOD_OK) {
        std::cerr << "Error initializing FMOD\n";
        return false;
    }

    result = system->init(32, FMOD_INIT_NORMAL, nullptr);
    return result == FMOD_OK;

}

void AudioManager::update() {
    if (system) {
        system->update();
    }
}

bool AudioManager::loadSound(const std::string& name, const std::string& filename, bool is3D, bool isLooping) {
    if (soundMap.find(name) != soundMap.end()) {
        std::cerr << "Sound file " << name << " already exists\n";
        return false;
    }

    FMOD_MODE mode = FMOD_DEFAULT;
    mode |= is3D ? FMOD_3D : FMOD_2D;
    mode |= isLooping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

    FMOD::Sound* sound = nullptr;
    FMOD_RESULT result = system->createSound(filename.c_str(), mode, nullptr, &sound);

    if (result != FMOD_OK) {
        std::cerr << "Error creating sound " << name << std::endl;
        return false;
    }

    soundMap[name] = sound;
    return true;
}

void AudioManager::playSound(const std::string& name) {
    auto it = soundMap.find(name);
    if (it != soundMap.end()) {
        FMOD::Channel* channel = nullptr;
        system->playSound(it->second, nullptr, false, &channel);
    }
}

void AudioManager::stopSound(const std::string& name) {

}

void AudioManager::cleanup() {
   for (auto& pair : soundMap) {
        if (pair.second) {
            pair.second->release();
        }
    }
    soundMap.clear();

    if (system) {
        system->close();
        system->release();
    }
}
