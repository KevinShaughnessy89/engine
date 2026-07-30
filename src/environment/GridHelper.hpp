#pragma once
#include "core/PrecompiledHeader.hpp"

class GridHelper {
   public:
    static glm::ivec2 worldToChunkIndex(glm::vec3 worldCoordinates);
    static glm::vec2 chunkIndexToWorldOrigin(int chunkX, int chunkZ);
};
