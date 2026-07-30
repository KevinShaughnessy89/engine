#pragma once

#include "CollisionAliases.hpp"

class CollisionObject;
class CollisionShape;

struct CollisionGeometryCandidatePair {
    CollisionObjectPtr object;
    std::vector<CollisionObjectPtr> staticGeometry;
};