#pragma once

#include "CollisionShapeGrid.hpp"
#include "components/collision/ShapeData.hpp"
#include "core/PrecompiledHeader.hpp"

class Chunk {
   public:
    CollisionShapeGrid collisionGrid;
    int startIndex;
    int endIndex;
    int minX, maxX, minZ, maxZ;
    float minHeight, maxHeight;

    Chunk(std::vector<uint32_t> triangles, int startIndex, int endIndex, int minX, int maxX,
          int minZ, int maxZ, float minHeight, float maxHeight);
};
