#pragma once

#include "core/PrecompiledHeader.hpp"

class MouseObserver {
   public:
    virtual ~MouseObserver() = default;
    virtual void mouseButtonEvent(int button, int action, int mods) {}
    virtual void mouseCursorEvent(double xposIn, double yposIn) {}
};