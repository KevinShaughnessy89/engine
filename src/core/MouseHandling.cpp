#include "core/PrecompiledHeader.hpp"
#include "core/MouseHandling.hpp"
#include "rendering/KinematicController.hpp"
#include "physics/ProjectileManager.hpp"
#include "MouseObserver.hpp"

void MouseHandling::addObserver(MouseObserver* observer) {
    observers.push_back(observer);
}

void MouseHandling::removeObserver(MouseObserver* observer) {
    observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
}

void MouseHandling::notify(MouseEventData data) {
    for (MouseObserver* observer : observers) {
        if (data.button != -1) {
            observer->mouseButtonEvent(data.button, data.action, data.mods);
        }
        else {
            observer->mouseCursorEvent(data.xposIn, data.yposIn);
        }
    }
}

void MouseHandling::mouseButton_callback(int button, int action, int mods) {
    MouseEventData data;
    data.button = button;
    data.action = action;
    data.mods = mods;
    notify(data);
}

void MouseHandling::mouseCursor_callback(double xposIn, double yposIn) {
    MouseEventData data;
    data.xposIn = xposIn;
    data.yposIn = yposIn;
    notify(data);
}

void MouseHandling::printObservers() {
    for (auto observer : observers) {
        std::cout << " - " << observer << std::endl;
    }
}
