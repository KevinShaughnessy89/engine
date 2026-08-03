#include "HeightMapGenerator.hpp"

#include <glm/ext/vector_float3.hpp>

#include "Biomes.hpp"
#include "EnvironmentState.hpp"
#include "TerrainConfig.hpp"
#include "Worley2D.hpp"
#include "rendering/DebugState.hpp"

/**
 * Terrain Generation Parameters:
 *
 * int octaves:
 *     - The number of layers of noise added together.
 *     - More octaves create more detail by stacking higher-frequency noise on top of
 * lower-frequency noise.
 *     - Too many octaves can slow down generation and make terrain look overly noisy.
 *
 * float persistence:
 *     - Controls how much each subsequent octave contributes to the final height.
 *     - A lower value (e.g., 0.3) makes higher octaves fade out faster, resulting in smoother
 * terrain.
 *     - A higher value (e.g., 0.7) keeps the detail from later octaves more pronounced, creating
 * rougher terrain.
 *
 * float lacunarity:
 *     - Determines how much the frequency increases between octaves.
 *     - A value > 1 increases frequency each octave, adding finer detail at each level.
 *     - For example, lacunarity = 2.0 doubles the frequency at each step.
 *
 * float baseFrequency:
 *     - The starting frequency of the noise.
 *     - Lower base frequencies result in larger, smoother terrain features.
 *     - Higher base frequencies lead to more compact and rapidly changing features.
 *
 * Together, these parameters shape the terrain's *scale*, *roughness*, and *detail level*.
 */

namespace HeightMapGenerator {

float pseudoNoise(float x, float z, float baseFreq) {
    float sum = 0.0f;
    sum += std::sin(x * baseFreq * 1.00f + z * baseFreq * 0.31f);
    sum += std::sin(x * baseFreq * 0.57f - z * baseFreq * 0.83f) * 0.6f;
    sum += std::sin(x * baseFreq * -0.72f + z * baseFreq * 0.44f) * 0.35f;
    return sum / (1.0f + 0.6f + 0.35f);  // normalize to roughly [-1, 1]
}

float getHeightAt(float worldX, float worldZ) {
    float moisture = pseudoNoise(worldX, worldZ, DebugState::moistureFrequency);
    float temperature = pseudoNoise(worldX, worldZ, DebugState::temperatureFrequency);

    std::vector<float> weights(biomes.size());
    float totalWeight = 0.0f;
    for (int i = 0; i < biomes.size(); i++) {
        float distance = glm::distance(glm::vec2(moisture, temperature),
                                       glm::vec2(biomes[i].targetX, biomes[i].targetZ));
        float sigma = DebugState::biomeSigma;
        weights[i] = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
        totalWeight += weights[i];
    }

    for (auto& weight : weights) weight /= totalWeight;  // normalize

    // Blend amplitude-like params ONLY. Frequencies/lacunarity must stay spatially uniform:
    // noise samples at x*f(x), so a varying f adds x*grad(f) to the local frequency -- confirmed
    // as the source of the tight sinusoidal ripples along biome transitions.
    float finalMacroAmplitude = 0.0f;
    float finalDetailAmplitude = 0.0f;
    float finalPersistence = 0.0f;
    float finalOctaves = 0.0f;
    for (int i = 0; i < biomes.size(); i++) {
        finalMacroAmplitude += weights[i] * biomes[i].macroAmplitude;
        finalDetailAmplitude += weights[i] * biomes[i].detailAmplitude;
        finalPersistence += weights[i] * biomes[i].persistence;
        finalOctaves += weights[i] * biomes[i].octaves;
    }

    float warpX = worldX + generateOctaveNoise(worldX, worldZ, 2, DebugState::warpFrequency, 0.5f,
                                               2.0f, 1.0f, &PerlinNoise::perlin2D) *
                               DebugState::warpStrength;
    float warpZ = worldZ + generateOctaveNoise(worldX + 2.0f / DebugState::warpFrequency,
                                               worldZ + 4.0f / DebugState::warpFrequency, 2,
                                               DebugState::warpFrequency, 0.5f, 2.0f, 1.0f,
                                               &PerlinNoise::perlin2D) *
                               DebugState::warpStrength;

    float lowland = generateOctaveNoiseSmooth(
        warpX, warpZ, finalOctaves, DebugState::lowlandFrequency, std::min(finalPersistence, 0.5f),
        2.0f, 1.0f, &PerlinNoise::perlin2D);

    float mountain =
        generateErosionFbm(warpX, warpZ, finalOctaves, DebugState::mountainFrequency,
                           std::min(finalPersistence, 0.5f), 2.0f, 1.0f, &PerlinNoise::perlin2D);

    mountain = std::copysign(std::pow(std::abs(mountain), DebugState::mountainSharpness), mountain);

    float mountainMicro =
        WorleyNoise::massifEnvelope(warpX, warpZ, DebugState::mountainMicroCellSize,
                                    DebugState::mountainMicroSmoothing,
                                    DebugState::mountainMicroSharpness) *
        DebugState::mountainDetailStrength;

    float mountainMicroRidge =
        WorleyNoise::worleyRidge(warpX, warpZ, DebugState::mountainMicroRidgeCellSize,
                                 DebugState::mountainMicroRidgeSharpness) *
        DebugState::mountainMicroRidgeStrength;

    mountain += mountainMicro + mountainMicroRidge;

    float h = (lowland + mountain) * finalMacroAmplitude * DebugState::debugAmplitudeScale;

    if (h < EnvironmentState::globalMinHeight) EnvironmentState::globalMinHeight = h;
    if (h > EnvironmentState::globalMaxHeight) EnvironmentState::globalMaxHeight = h;

    return h;
}

float generateOctaveNoise(float x, float z, int octaves, float baseFrequency, float persistence,
                          float lacunarity, float amplitude, float (*noiseFn)(float, float)) {
    float total = 0.0f;
    float frequency = baseFrequency;
    float maxValue = 0.0f;  // Used for normalizing result to [-1, 1]

    for (int i = 0; i < octaves; i++) {
        float rawX = x;
        float rawZ = z;

        float scaledX = rawX * frequency;
        float scaledZ = rawZ * frequency;

        float rawNoise = noiseFn(scaledX, scaledZ);

        float scaledNoise = rawNoise * amplitude;

        total += scaledNoise;

        maxValue += amplitude;

        amplitude *= persistence;

        frequency *= lacunarity;
    }

    float result = total / maxValue;
    return result;
}

// Erosion-flavored fBm (after Quilez): dampens each octave by the accumulated
// gradient magnitude of prior octaves, so steep areas resist piling on more
// detail while gentle areas keep texturing freely. Approximates the analytic
// derivative with central differences since perlin2D doesn't expose one.
// Fractional `octaves` fades the last layer in, matching generateOctaveNoiseSmooth,
// so biome-blended octave counts don't seam at integer crossings.
float generateErosionFbm(float x, float z, float octaves, float baseFrequency, float persistence,
                         float lacunarity, float amplitude, float (*noiseFn)(float, float)) {
    constexpr float kEps = 0.01f;  // small step in noise-domain units, not world units

    int fullOctaves = static_cast<int>(octaves);
    float lastOctaveFade = octaves - fullOctaves;

    float frequency = baseFrequency;
    float total = 0.0f;
    float maxValue = 0.0f;
    glm::vec2 dsum(0.0f);  // accumulated gradient from prior octaves

    auto sampleOctave = [&](float weight) {
        float dx = x * frequency;
        float dz = z * frequency;

        float n = noiseFn(dx, dz);
        float gx = (noiseFn(dx + kEps, dz) - noiseFn(dx - kEps, dz)) / (2.0f * kEps);
        float gz = (noiseFn(dx, dz + kEps) - noiseFn(dx, dz - kEps)) / (2.0f * kEps);

        float damping = 1.0f / (1.0f + glm::dot(dsum, dsum));
        total += amplitude * n * damping * weight;
        maxValue += amplitude * weight;

        dsum += glm::vec2(gx, gz) * frequency * weight;

        amplitude *= persistence;
        frequency *= lacunarity;
    };

    for (int i = 0; i < fullOctaves; i++) sampleOctave(1.0f);
    if (lastOctaveFade > 0.0f) sampleOctave(lastOctaveFade);

    return maxValue > 0.0f ? total / maxValue : 0.0f;
}

// Musgrave's ridged-multifractal: each octave is folded to a ridge (offset - |signal|, squared)
// then weighted by the previous octave's ridge strength, so ridges beget sharper ridges nearby
// while valleys stay smooth. Fractional `octaves` fades the last layer in, matching the other
// accumulators here, so biome-blended octave counts don't seam at integer crossings.
float generateRidgedMultifractal(float x, float z, float octaves, float baseFrequency,
                                 float persistence, float lacunarity, float amplitude,
                                 float (*noiseFn)(float, float)) {
    constexpr float kOffset = 1.0f;   // fold distance from the ridge line; higher flattens ridges
    constexpr float kGain = 0.0001f;  // how strongly a ridge amplifies the next octave's weight

    // octaves: layer count; fractional part fades the last layer in so blended counts don't pop
    int fullOctaves = static_cast<int>(octaves);
    float lastOctaveFade = octaves - fullOctaves;

    // baseFrequency: starting sample frequency -- lower spreads ridges out, higher packs them in
    float frequency = baseFrequency;
    float total = 0.0f;
    float maxValue = 0.0f;
    float weight = 1.0f;  // carries the previous octave's ridge strength into the next octave

    auto sampleOctave = [&](float octaveWeight) {
        float signal = noiseFn(x * frequency, z * frequency);
        signal = kOffset - std::abs(signal);  // fold: zero-crossings of the raw noise become peaks
        signal *= signal;                     // square: sharpens ridges, widens valleys
        signal *= weight;                     // scale by inherited ridge strength from last octave

        weight = std::clamp(signal * kGain, 0.0f, 1.0f);

        total += signal * amplitude * octaveWeight;
        maxValue += amplitude * octaveWeight;

        amplitude *= persistence;  // how much amplitude shrinks per octave; lower favors low freq
        frequency *= lacunarity;   // how much frequency grows per octave
    };

    for (int i = 0; i < fullOctaves; i++) sampleOctave(1.0f);
    if (lastOctaveFade > 0.0f) sampleOctave(lastOctaveFade);

    return maxValue > 0.0f ? total / maxValue : 0.0f;
}

// Fades in a fractional last octave so the biome-blended octave count varies without seams.
float generateOctaveNoiseSmooth(float x, float z, float octaves, float baseFrequency,
                                float persistence, float lacunarity, float amplitude,
                                float (*noiseFn)(float, float)) {
    int fullOctaves = static_cast<int>(octaves);
    float lastOctaveFade = octaves - fullOctaves;
    float total = 0.0f;
    float frequency = baseFrequency;
    float maxValue = 0.0f;
    float weight = 1.0f;
    float gain = 2.0f;

    for (int i = 0; i < fullOctaves; i++) {
        float noise = noiseFn(x * frequency, z * frequency) * amplitude;
        total += noise;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    if (lastOctaveFade > 0.0f) {
        total += noiseFn(x * frequency, z * frequency) * amplitude * lastOctaveFade;
        maxValue += amplitude * lastOctaveFade;
    }

    return maxValue > 0.0f ? total / maxValue : 0.0f;
}

float bilerpHeightAt(const std::vector<float>& heightMap, int size, float x, float z, int minX,
                     int minZ) {
    // Chunk heightmaps sample every gridSize/triangleCount units, with a 1-sample border row at
    // index 0 (see TerrainMeshGenerator::generateHeightMap's loop from -1).
    float spacing = static_cast<float>(TerrainConfig::gridSize) / TerrainConfig::triangleCount;
    float heightMapX = (x - minX) / spacing + 1.0f;
    float heightMapZ = (z - minZ) / spacing + 1.0f;

    // Get the integer coordinates of the top-left texel, clamped so x0+1/z0+1 stay in bounds.
    int x0 = std::clamp(static_cast<int>(std::floor(heightMapX)), 0, size - 2);
    int z0 = std::clamp(static_cast<int>(std::floor(heightMapZ)), 0, size - 2);

    // Calculate fractional offsets (0.0 to 1.0)
    float fx = std::clamp(heightMapX - std::floor(heightMapX), 0.0f, 1.0f);
    float fz = std::clamp(heightMapZ - std::floor(heightMapZ), 0.0f, 1.0f);

    // Sample the four corner heights
    float h00 = heightMap[z0 * size + x0];            // top-left
    float h10 = heightMap[z0 * size + x0 + 1];         // top-right
    float h01 = heightMap[(z0 + 1) * size + x0];       // bottom-left
    float h11 = heightMap[(z0 + 1) * size + x0 + 1];   // bottom-right

    // Interpolate on whichever triangle plane the mesh actually uses for this quad (split along
    // the topRight-bottomLeft diagonal, matching generateTerrainTriangles), instead of a bilinear
    // blend of all 4 corners -- bilinear introduces a curved fx*fz term that diverges from the
    // flat mesh triangles whenever the corners aren't coplanar, causing grounding to float/sink.
    if (fx + fz <= 1.0f) {
        return h00 + fx * (h10 - h00) + fz * (h01 - h00);
    }
    return h11 + (1.0f - fx) * (h01 - h11) + (1.0f - fz) * (h10 - h11);
}

glm::vec3 getNormalAt(float worldX, float worldZ, float epsilon) {
    float hL = getHeightAt(worldX - epsilon, worldZ);
    float hR = getHeightAt(worldX + epsilon, worldZ);
    float hD = getHeightAt(worldX, worldZ - epsilon);
    float hU = getHeightAt(worldX, worldZ + epsilon);

    float dhdx = (hR - hL) / (2.0f * epsilon);
    float dhdz = (hU - hD) / (2.0f * epsilon);

    return glm::normalize(glm::vec3(-dhdx, 1.0f, -dhdz));
}

void laplacianSmooth(std::vector<float>& heightMap, int size, int iterations, float lambda) {
    auto idx = [size](int i, int j) { return i * size + j; };
    std::vector<float> smoothed = heightMap;

    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 1; i < size - 1; i++) {
            for (int j = 1; j < size - 1; j++) {
                double neighborAverage = (smoothed[idx(i - 1, j)] + smoothed[idx(i + 1, j)] +
                                          smoothed[idx(i, j - 1)] + smoothed[idx(i, j + 1)]) /
                                         4.0;
                heightMap[idx(i, j)] += lambda * (neighborAverage - smoothed[idx(i, j)]);
            }
        }
        smoothed = heightMap;
    }
}

// Thermal erosion (talus-angle / angle-of-repose model): each cell slides a fraction of
// its "excess" height (anything steeper than the talus angle) onto downhill neighbors.
// Purely local (4-neighbor, single-cell reach per pass) -- unlike hydraulic erosion this
// has no whole-region dependency, so it's safe to run on streamed/infinite chunks with
// only a small halo (halo width = iteration count, since each pass only reaches 1 cell).
void HeightMapGenerator::thermalErode(std::vector<float>& heightMap, int size, int iterations) {
    int cellSize = TerrainConfig::gridSize / TerrainConfig::triangleCount;  // world units per cell
    float talusAngleDegrees = 30.0f;  // typical angle of repose for dry sand/soil
    float erosionRate = 0.1f;         // fraction of excess height to move per iteration

    // Max height difference allowed between adjacent cells before material slides downhill.
    float maxDiff = cellSize * std::tan(glm::radians(talusAngleDegrees));

    constexpr int dx[4] = {-1, 1, 0, 0};
    constexpr int dz[4] = {0, 0, -1, 1};

    auto idx = [size](int i, int j) { return i * size + j; };
    std::vector<float> delta(size * size, 0.0f);

    for (int iter = 0; iter < iterations; iter++) {
        std::fill(delta.begin(), delta.end(), 0.0f);

        for (int i = 1; i < size - 1; i++) {
            for (int j = 1; j < size - 1; j++) {
                float h = heightMap[idx(i, j)];

                // Sum excess height over the talus threshold across every steeper downhill
                // neighbor, so material splits proportionally instead of dumping onto
                // whichever neighbor happens to be steepest.
                float excess[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                float totalExcess = 0.0f;

                for (int n = 0; n < 4; n++) {
                    float nh = heightMap[idx(i + dx[n], j + dz[n])];
                    float diff = h - nh;
                    if (diff > maxDiff) {
                        excess[n] = diff - maxDiff;
                        totalExcess += excess[n];
                    }
                }

                if (totalExcess <= 0.0f) continue;

                for (int n = 0; n < 4; n++) {
                    if (excess[n] <= 0.0f) continue;
                    // 0.5 factor: only move half the excess per pass so slopes settle
                    // gradually toward the talus angle rather than overshooting/oscillating.
                    float amount = excess[n] * erosionRate * 0.5f;
                    delta[idx(i, j)] -= amount;
                    delta[idx(i + dx[n], j + dz[n])] += amount;
                }
            }
        }

        for (int k = 0; k < size * size; k++) heightMap[k] += delta[k];
    }
}

}  // namespace HeightMapGenerator
