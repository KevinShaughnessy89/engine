#pragma once
#include <cstdint>

#include "core/PrecompiledHeader.hpp"

namespace PerlinNoise {
constexpr int PERM_SIZE = 256;
constexpr int NUM_GRADIENTS = 12;

extern uint32_t seed;
extern uint8_t perm[2 * PERM_SIZE];

float smoothstep(float t);
void initPermutationTable(uint32_t seed);
void setSeed(uint32_t seed);
float lerp(float a, float b, float t);
float perlin2D(float x, float z);
float ridgeNoise(float x, float z);
float fade(float t);
float grad(int hash, float x, float z);
};  // namespace PerlinNoise
