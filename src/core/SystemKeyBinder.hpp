#pragma once

#include "core/KeyObserver.hpp"
#include "core/PrecompiledHeader.hpp"

class KeyHandling;

class SystemKeyBinder : public KeyObserver {
   public:
    SystemKeyBinder();
    ~SystemKeyBinder();

    void onKeyEvent(int key, int scancode, int action, int mods) override;
    void onContinuousInput(double deltaTime, const KeyHandling& keyHandler) override;
    void toggleDebugPanel();

   private:
};
