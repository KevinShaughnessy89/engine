#pragma once
#include "TerrainConfig.hpp"

namespace WorleyNoise {

struct WorleyResult {
    float f1;  // Distance to the closest feature point
    float f2;  // Distance to the second closest feature point
    glm::ivec2 gridCoords;
};

inline glm::vec2 cellFeaturePoint(int cellX, int cellZ) {
    uint32_t hash = static_cast<uint32_t>(cellX) * 374761393u +
                    static_cast<uint32_t>(cellZ) * 668265263u + TerrainConfig::seed * 2246822519u;
    hash = (hash ^ (hash >> 13)) * 1274126177u;
    hash ^= (hash >> 16);

    float fx = (hash & 0xFFFF) / 65536.0f;          // Normalize to [0, 1]
    float fz = ((hash >> 16) & 0xFFFF) / 65536.0f;  // Normalize to [0, 1]

    return glm::vec2(cellX + fx, cellZ + fz);
}

// Distances are returned in world units; cellSize sets the feature-point spacing.
inline WorleyResult getDistancesResult(float x, float z, float cellSize) {
    float cx = x / cellSize;
    float cz = z / cellSize;

    int cellX = static_cast<int>(std::floor(cx));
    int cellZ = static_cast<int>(std::floor(cz));

    float minDist1 = std::numeric_limits<float>::max();
    float minDist2 = std::numeric_limits<float>::max();

    glm::ivec2 closestCell;

    for (int offsetX = -1; offsetX <= 1; ++offsetX) {
        for (int offsetZ = -1; offsetZ <= 1; ++offsetZ) {
            int neighborX = cellX + offsetX;
            int neighborZ = cellZ + offsetZ;

            glm::vec2 featurePoint = cellFeaturePoint(neighborX, neighborZ);

            float dist = glm::distance(glm::vec2(cx, cz), featurePoint);

            if (dist < minDist1) {
                minDist2 = minDist1;
                minDist1 = dist;
                closestCell = glm::ivec2(neighborX, neighborZ);
            } else if (dist < minDist2) {
                minDist2 = dist;
            }
        }
    }

    return {minDist1 * cellSize, minDist2 * cellSize, closestCell};
}

inline float amplitudeForCell(glm::ivec2 gridCoords) {
    uint32_t hash = static_cast<uint32_t>(gridCoords.x) * 374761393u +
                    static_cast<uint32_t>(gridCoords.y) * 668265263u +
                    TerrainConfig::seed * 2246822519u;
    hash = (hash ^ (hash >> 13)) * 1274126177u;
    hash ^= (hash >> 16);

    // Keep the division in float: assigning it back into the uint32 truncated every cell to 0.
    return (hash & 0xFFFF) / 65536.0f;
}

// Smooth [0,1] dome per cell; smoothing rounds Voronoi-boundary creases, sharpness narrows domes.
// Skips amplitudeForCell() on purpose (it steps at boundaries); modulate with smooth noise instead.
// Takes raw world coordinates (not pre-scaled by an fBm frequency loop) -- cellSize IS the
// frequency here, so call this directly rather than through generateErosionFbm/generateOctaveNoise.
inline float massifEnvelope(float x, float z, float cellSize, float smoothing, float sharpness) {
    WorleyResult result = getDistancesResult(x, z, cellSize);
    float d1 = result.f1 / cellSize;
    float d2 = result.f2 / cellSize;
    // F1^2 instead of F1: linear distance cone-kinks at the feature point, squared stays smooth.
    float dd = glm::mix(d1 * d1, 0.5f * (d1 * d1 + d2 * d2), smoothing);
    return std::pow(1.0f - glm::smoothstep(0.0f, 1.0f, std::clamp(dd, 0.0f, 1.0f)), sharpness);
}

// Cellular ridge network in [0,1]: 1 along Voronoi boundaries (F1 == F2), 0 in cell interiors.
inline float worleyRidge(float x, float z, float cellSize, float sharpness) {
    WorleyResult result = getDistancesResult(x, z, cellSize);
    float ridge = 1.0f - std::clamp((result.f2 - result.f1) / cellSize, 0.0f, 1.0f);
    return std::pow(ridge, sharpness);
}
}  // namespace WorleyNoise
