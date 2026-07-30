#pragma once
#include "core/PrecompiledHeader.hpp"

struct Biome {
    float targetX, targetZ;
    float macroAmplitude;
    float macroFrequency;
    float detailAmplitude;
    float microFrequency;
    float persistence;
    int octaves;
    float lacunarity;
    bool isMountain = false;  // currently unused by getHeightAt()
};

// targetX/targetZ are compared against Perlin noise samples normalized to [-1, 1] (see
// HeightMapGenerator::getHeightAt()'s moisture/temperature calls), so every target here is scaled
// to stay within that range (max magnitude 0.9) instead of the unreachable 1.2 the quadrants used
// to reach for.
// Defined in Biomes.cpp -- kept out of the header so tuning these values only recompiles that one
// TU instead of everything that includes Biomes.hpp.
extern std::vector<Biome> biomes;