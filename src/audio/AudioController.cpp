#include "AudioController.hpp"

void AudioController::init() {
    FMOD_RESULT result = FMOD::System_Create(&system);

    if (result != FMOD_OK) {
        std::cerr << "Error initializing FMOD\n";
        return;
    }

    result = system->init(32, FMOD_INIT_NORMAL, nullptr);
}

void AudioController::update(double deltaTime) {
    if (system) {
        system->update();
    }

    for (auto& [name, sound] : soundMap) {
        if (sound.loopFrequency > 0.0f) {
            sound.timeSinceLastPlay += deltaTime;
            if (sound.timeSinceLastPlay >= sound.loopFrequency) {
                playSound(name);
                sound.timeSinceLastPlay = 0.0f;
            }
        }
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

    soundMap[name] = Sound{name, sound, is3D, isLooping};
}

void AudioController::playSound(const std::string& name) {
    auto it = soundMap.find(name);
    if (it != soundMap.end()) {
        FMOD::Channel* channel = nullptr;
        system->playSound(it->second.soundPtr, nullptr, false, &channel);
        if (channel) {
            it->second.activeChannels.push_back(channel);
        }
    }
}

void AudioController::stopSound(const std::string& name) {
    auto it = soundMap.find(name);
    if (it == soundMap.end()) return;

    for (FMOD::Channel* ch : it->second.activeChannels) {
        if (ch) ch->stop();
    }
    it->second.activeChannels.clear();
}

void AudioController::cleanup() {
    for (auto& pair : soundMap) {
        if (pair.second.soundPtr) {
            pair.second.soundPtr->release();
            // delete pair.second; // Not needed, as pair.second is not a pointer
            pair.second.soundPtr = nullptr;
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
