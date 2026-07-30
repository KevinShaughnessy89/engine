#pragma once
#include "PerlinNoise.hpp"
#include "core/PrecompiledHeader.hpp"

// Stateless free functions (the seed lives in PerlinNoise), so no instances are needed.
namespace HeightMapGenerator {
float generateOctaveNoise(float x, float z, int octaves, float baseFrequency, float persistence,
                          float lacunarity, float amplitude, float (*noiseFn)(float, float));
float generateOctaveNoiseSmooth(float x, float z, float octaves, float baseFrequency,
                                float persistence, float lacunarity, float amplitude,
                                float (*noiseFn)(float, float));
float generateErosionFbm(float x, float z, float octaves, float baseFrequency, float persistence,
                         float lacunarity, float amplitude, float (*noiseFn)(float, float));
float generateRidgedMultifractal(float x, float z, float octaves, float baseFrequency,
                                 float persistence, float lacunarity, float amplitude,
                                 float (*noiseFn)(float, float));
// heightMap is a flat size*size grid; laplacianSmooth/thermalErode are isotropic 4-neighbor
// stencils, so which axis is "row" vs "column" doesn't affect their result.
void laplacianSmooth(std::vector<float>& heightMap, int size, int iterations, float lambda);
void thermalErode(std::vector<float>& heightMap, int size, int iterations);
// Samples a chunk's baked heightmap: flat (triangleCount+3)^2 grid with a 1-sample border at
// index 0, GL row-major (index = z * size + x).
float bilerpHeightAt(const std::vector<float>& heightMap, int size, float x, float z, int minX,
                     int minZ);
float getHeightAt(float worldX, float worldZ);
glm::vec3 getNormalAt(float worldX, float worldZ, float epsilon = 0.01f);
}  // namespace HeightMapGenerator
