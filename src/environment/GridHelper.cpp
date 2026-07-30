#include "GridHelper.hpp"

#include "TerrainConfig.hpp"

glm::ivec2 GridHelper::worldToChunkIndex(glm::vec3 worldCoordinates) {
    return {static_cast<int>(std::floor(worldCoordinates.x / TerrainConfig::gridSize)),
            static_cast<int>(std::floor(worldCoordinates.z / TerrainConfig::gridSize))};
}

glm::vec2 GridHelper::chunkIndexToWorldOrigin(int chunkX, int chunkZ) {
    return {chunkX * TerrainConfig::gridSize, chunkZ * TerrainConfig::gridSize};
}
