#pragma once
#include "core/PrecompiledHeader.hpp"

class CollisionObject;

struct BroadphaseCandidateSingle {
    std::unique_ptr<CollisionObject> object;
};