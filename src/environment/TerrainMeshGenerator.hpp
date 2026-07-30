#pragma once
#include "EnvironmentState.hpp"
#include "TerrainConfig.hpp"
#include "TriangleEntityBuilder.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "environment/TerrainMeshGeneratorStructs.hpp"
#include "rendering/Model.hpp"

class TerrainTriangle;
class Texture;
class Model;
class Terrain;
class ChunkModel;

namespace TerrainMeshGenerator {

extern uint32_t currentSeed;

void init();

std::unique_ptr<Model> createModelFromMeshData(TerrainMeshData& meshData);

TerrainMeshData calculateMeshData(const std::vector<glm::vec3>& vertices, int minX, int minZ,
                                  int maxX, int maxZ, glm::ivec2 gridCoords);

TerrainMeshData generateMeshData(int gridSize, int minX, int maxX, int minZ, int maxZ,
                                 glm::ivec2 gridCoords, bool buildCollision = true);

inline void setWorldSize(const glm::vec2& min, const glm::vec2& max) {
    EnvironmentState::worldMin = min;
    EnvironmentState::worldMax = max;
}

void generateSeed();

// heightMap is the flat (triangleCount+3)^2 grid described on TerrainMeshData::heightMap.
std::vector<std::vector<glm::vec3>> generateVertices(int minX, int maxX, int minZ, int maxZ,
                                                      const std::vector<float>& heightMap);

std::vector<unsigned int> generateStripIndices();
std::vector<unsigned int> generateQuadIndices();
std::vector<glm::vec2> generateNormalizedUVs(const std::vector<glm::vec3>& vertices);

void generateHeightMap(TerrainMeshData& data, int minX, int minZ, int maxX, int maxZ);

// Sobel-gradient normal (XZ only, Y reconstructed in-shader) for every interior texel of a
// halo-padded heightmap of width x width. Output is (width-2) x (width-2), matching the unpadded
// grid. Must stay numerically identical to the formula in terrain.tese/gbufferTerrain.tese.
std::vector<glm::vec2> calculateNormals(const std::vector<float>& heightMap, int width,
                                        float worldTexelSize);

std::vector<GLuint> generateHeightMapTextures(const std::vector<float>& heightMapData);

std::vector<TriangleEntityBuilder> generateTerrainTriangles(const std::vector<glm::vec3>& vertices);

}  // namespace TerrainMeshGenerator
