#include "SystemKeyBinder.hpp"

#include "core/Game.hpp"
#include "core/KeyHandling.hpp"

void SystemKeyBinder::toggleDebugPanel() {
    Game::showDebugPanel = !Game::showDebugPanel;
}

void SystemKeyBinder::onKeyEvent(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS) {
        toggleDebugPanel();
    }
}

void SystemKeyBinder::onContinuousInput(double deltaTime, const KeyHandling& keyHandler) {
}

SystemKeyBinder::~SystemKeyBinder() {
}

SystemKeyBinder::SystemKeyBinder() {
}
