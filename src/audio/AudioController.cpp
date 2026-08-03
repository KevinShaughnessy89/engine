#include "AudioController.hpp"

#include "core/PrecompiledHeader.hpp"

AudioController::AudioController() : system(nullptr) {
}

AudioController::~AudioController() {
    cleanup();
}

void AudioController::init() {
    FMOD_RESULT result = FMOD::System_Create(&system);

    if (result != FMOD_OK) {
        std::cerr << "Error initializing FMOD\n";
        return;
    }

    result = system->init(32, FMOD_INIT_NORMAL, nullptr);
}

void AudioController::update() {
    if (system) {
        system->update();
    }
}

void AudioController::loadSound(const std::string& name, const std::string& filename, bool is3D,
                                bool isLooping) {
    if (soundMap.find(name) != soundMap.end()) {
        std::cerr << "Sound file " << name << " load attempt but it already exists.\n";
        return;
    }

    FMOD_MODE mode = FMOD_DEFAULT;
    mode |= is3D ? FMOD_3D : FMOD_2D;
    mode |= isLooping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

    FMOD::Sound* sound = nullptr;
    FMOD_RESULT result = system->createSound(filename.c_str(), mode, nullptr, &sound);

    if (result != FMOD_OK) {
        std::cerr << "Error creating sound " << name << std::endl;
        return;
    }

    soundMap[name] = sound;
}

void AudioController::playSound(const std::string& name) {
    auto it = soundMap.find(name);
    if (it != soundMap.end()) {
        FMOD::Channel* channel = nullptr;
        system->playSound(it->second, nullptr, false, &channel);
    }
}

void AudioController::stopSound(const std::string& name) {
}

void AudioController::cleanup() {
    for (auto& pair : soundMap) {
        if (pair.second) {
            pair.second->release();
            delete pair.second;
            pair.second = nullptr;
        }
    }
    soundMap.clear();

    if (system) {
        system->close();
        system->release();
        delete system;
        system = nullptr;
    }
}
