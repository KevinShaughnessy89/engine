#include "Chunk.hpp"

#include <limits>

#include "collision/shapes/Triangle.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"

Chunk::Chunk(std::vector<uint32_t> triangles, int startIndex, int endIndex, int minX, int maxX,
             int minZ, int maxZ, float minHeight, float maxHeight)
    : startIndex(startIndex),
      endIndex(endIndex),
      minX(minX),
      maxX(maxX),
      minZ(minZ),
      maxZ(maxZ),
      minHeight(minHeight),
      maxHeight(maxHeight) {
    glm::vec3 min(std::numeric_limits<float>::max());
    glm::vec3 max(std::numeric_limits<float>::lowest());

    collisionGrid = CollisionShapeGrid(glm::vec2(min.x, min.z), glm::vec2(max.x, max.z), triangles);
}
