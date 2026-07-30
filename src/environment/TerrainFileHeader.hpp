#pragma once
#include "core/PrecompiledHeader.hpp"

#include <stdint.h>

struct TerrainFileHeader {
    uint32_t magic;  // 'TERR'
    uint32_t vertexCount;
    uint32_t triangleCount;
    uint32_t seed;
    uint32_t chunkCount;
    uint32_t gridSize;
    uint32_t worldMinX;
    uint32_t worldMaxX;
    uint32_t worldMinZ;
    uint32_t worldMaxZ;
};

struct TerrainFileIndexEntry {
    int32_t gridX;
    int32_t gridZ;
    uint64_t dataOffset;
    uint64_t dataSize;
};

constexpr uint32_t TERRAIN_MAGIC = 0x52524554;
