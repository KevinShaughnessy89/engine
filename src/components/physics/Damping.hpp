#pragma once

#include "core/PrecompiledHeader.hpp"

struct Damping {
    // 1 = no damping (velocity is multiplied by pow(damping, dt) each tick,
    // so 0 would zero velocity instantly).
    float linear = 1.0f;
    float angular = 1.0f;
};