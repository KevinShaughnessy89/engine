#include "PerlinNoise.hpp"

#include "core/PrecompiledHeader.hpp"

#define ENABLE_LOGGING
#include <common/log.hpp>

namespace PerlinNoise {
uint32_t seed = 0;
uint8_t perm[2 * PERM_SIZE];

void initPermutationTable(uint32_t seed_) {
    std::vector<uint8_t> p(PERM_SIZE);
    for (int i = 0; i < PERM_SIZE; ++i) {
        p[i] = i;
    }

    std::mt19937 gen(seed_);
    std::shuffle(p.begin(), p.end(), gen);

    for (int i = 0; i < PERM_SIZE; ++i) {
        perm[i] = p[i];
        perm[i + PERM_SIZE] = p[i];
    }
}

float smoothstep(float t) {
    return t * t * (3 - 2 * t);
}

float lerp(float a, float b, float t) {
    float result = a + t * (b - a);
    return result;
}

float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

// 8 unit-length gradients (4 axial + 4 diagonal); the old 4-diagonal set banded diagonally.
float grad(int hash, float x, float z) {
    constexpr float D = 0.70710678f;
    switch (hash & 7) {
        case 0:
            return x;
        case 1:
            return -x;
        case 2:
            return z;
        case 3:
            return -z;
        case 4:
            return D * (x + z);
        case 5:
            return D * (-x + z);
        case 6:
            return D * (x - z);
        case 7:
            return D * (-x - z);
    }
    return 0;
}

float ridgeNoise(float x, float z) {
    // Ridge noise transformation
    float rawNoise = perlin2D(x, z);
    float ridgeNoise = 1.0f - std::abs(rawNoise);
    ridgeNoise *= ridgeNoise;  // Square to sharpen ridges
    return ridgeNoise;
}

void setSeed(uint32_t newSeed) {
    seed = newSeed;
    LOG("SEED SET: " << seed << std::endl);
    initPermutationTable(seed);
}

float perlin2D(float x, float z) {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;

    float xf = x - std::floor(x);
    float zf = z - std::floor(z);

    float u = fade(xf);
    float w = fade(zf);

    int A = perm[X] + Z;
    int AA = perm[A];
    int AB = perm[A + 1];
    int B = perm[X + 1] + Z;
    int BA = perm[B];
    int BB = perm[B + 1];

    float gAA = grad(AA, xf, zf);
    float gBA = grad(BA, xf - 1, zf);
    float gAB = grad(AB, xf, zf - 1);
    float gBB = grad(BB, xf - 1, zf - 1);

    float lerpX1 = lerp(gAA, gBA, u);
    float lerpX2 = lerp(gAB, gBB, u);
    // Unit gradients bound 2D Perlin to +-sqrt(2)/2; sqrt(2) fills [-1,1], no clamp plateaus.
    return lerp(lerpX1, lerpX2, w) * 1.41421356f;
}
}  // namespace PerlinNoise

float cubicInterpolate(float p0, float p1, float p2, float p3, float t) {
    float a0 = p3 - p2 - p0 + p1;
    float a1 = p0 - p1 - a0;
    float a2 = p2 - p0;
    float a3 = p1;
    return a0 * t * t * t + a1 * t * t + a2 * t + a3;
}
