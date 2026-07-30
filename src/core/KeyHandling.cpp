#include "core/PrecompiledHeader.hpp"
#include "core/KeyHandling.hpp"

#include "core/KeyObserver.hpp"
#include "physics/ProjectileManager.hpp"
#include "rendering/KinematicController.hpp"

void KeyHandling::addObserver(KeyObserver* observer) {
    observers.push_back(observer);
}

void KeyHandling::removeObserver(KeyObserver* observer) {
    observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
}

void KeyHandling::processContinousInput(double deltaTime) {
    for (KeyObserver* observer : observers) {
        observer->onContinuousInput(deltaTime, *this);
    }
}

bool KeyHandling::isKeyPressed(int key) const {
    auto it = keyStates.find(key);
    return it != keyStates.end() && it->second;
}

void KeyHandling::keyboard_callback(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        keyStates[key] = true;
    } else if (action == GLFW_RELEASE) {
        keyStates[key] = false;
    }

    for (KeyObserver* observer : observers) {
        observer->onKeyEvent(key, scancode, action, mods);
    }
}

void KeyHandling::printObservers() {
    std::cout << "Current observers:" << std::endl;
    for (auto observer : observers) {
        std::cout << " - " << observer << std::endl;
    }
}
