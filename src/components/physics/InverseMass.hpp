#pragma once

#include "core/PrecompiledHeader.hpp"

struct InverseMass {
    // 0 = infinite mass: an entity whose mass was never set cannot be
    // accelerated by forces.
    float value = 0.0f;
};