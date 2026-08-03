#pragma once

#include <cstdint>

struct TerrainConfig {
    // Mutable at runtime via the debug panel's Terrain tab (ChunkManager::regenerateTerrain()
    // rebuilds every GL/collision structure sized off this value when it changes).
    inline static int triangleCount = 32;
    inline static constexpr bool useTriangleStrips = true;
    inline static constexpr int gridSize = 512;
    inline static constexpr int gridResolution = 2;
    inline static int lookUpRadius = 2;
    inline static constexpr uint32_t seed = 1337u;
    // Chunks within this radius stream in render-only (heightmap + LOD2 mesh, no collision);
    // collision triangles and full LOD meshes exist only inside lookUpRadius (see ChunkManager).
    // additive/multiplicative factors are hysterisis terms to avoid thrashing
    inline static int renderRadius = 8;
    inline static int collisionCapacity =
        (4 * TerrainConfig::lookUpRadius) * (4 * TerrainConfig::lookUpRadius);
    inline static int renderCapacity =
        (2 * (TerrainConfig::renderRadius + 2)) * (2 * (TerrainConfig::renderRadius + 2));
    inline static int maxChunks = collisionCapacity + renderCapacity;
    // World-space distances at which ChunkModel::render switches LOD tiers (see
    // desiredLodForDistance in ChunkModel.cpp). Defaults match the original
    // 0.25/0.5 * (2 * lookUpRadius * gridSize) fractions; editable live from the debug panel's
    // Terrain tab, no regeneration needed since only the render-time tier pick reads these.
    inline static float lod1Distance = 512.0f;
    inline static float lod2Distance = 1024.0f;

    // World-space distance beyond which a tree switches from real geometry to its billboard
    // imposter.
    inline static float treeImposterDistanceLOD = 256.0f;
};