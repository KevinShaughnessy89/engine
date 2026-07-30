#pragma once

// Sidecar for the InstancedRenderable on the same entity -- carries the one piece of data that's
// specific to imposters and meaningless for every other instanced model.
#include <limits>

struct TerrainTreeImposter {
    int gridRes = 8;
    int type = std::numeric_limits<int>::max();
};
