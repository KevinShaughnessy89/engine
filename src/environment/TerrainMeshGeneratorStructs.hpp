#pragma once
#include <limits>

#include "TriangleEntityBuilder.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/MeshStructs.hpp"

class Texture;
class TerrainTriangle;

struct TerrainMeshData {
    int minX, minZ, maxX, maxZ;
    glm::ivec2 gridCoords;
    uint32_t currentSeed;
    // False for render-only chunks outside the collision window: no triangles are generated and
    // createChunk uploads only the LOD2 mesh.
    bool buildCollision = true;
    std::vector<glm::vec3> meshVertices_lod0;
    std::vector<glm::vec3> meshVertices_lod1;
    std::vector<glm::vec3> meshVertices_lod2;
    std::vector<unsigned int> indices;
    // Flat (triangleCount+3)^2 grid, 1-sample border at index 0. OpenGL/GL-texture row-major
    // convention: z is the row, x is the contiguous column -- index = z * width + x (see
    // TerrainMeshGenerator::generateHeightMap).
    std::vector<float> heightMap;
    // Same layout/width as heightMap. XZ components of the Sobel-derived per-texel normal (see
    // TerrainMeshGenerator::calculateNormals); Y is reconstructed in-shader as
    // sqrt(1 - x*x - z*z), which is unambiguous since heightfield-derived normals always have a
    // positive Y (slope never reaches 90 degrees).
    std::vector<glm::vec2> normalMap;
    std::vector<Texture*> textures;
    std::vector<TriangleEntityBuilder> triangles;
    float minHeight = std::numeric_limits<float>::max();
    float maxHeight = std::numeric_limits<float>::lowest();
};

struct ChunkData {
    int minX, maxX, minZ, maxZ;

    ChunkData(int triangleCount, int minX, int maxX, int minZ, int maxZ)
        : minX(minX), maxX(maxX), minZ(minZ), maxZ(maxZ) {}
};
