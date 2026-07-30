#pragma once

#include "core/KeyHandling.hpp"
#include "core/PrecompiledHeader.hpp"

class KeyObserver {
   public:
    virtual ~KeyObserver() = default;
    virtual void onKeyEvent(int key, int scancode, int action, int mods) = 0;
    virtual void onContinuousInput(double deltaTime, const KeyHandling& keyHandler) = 0;
};