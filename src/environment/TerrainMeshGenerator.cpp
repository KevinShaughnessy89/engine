#include "TerrainMeshGenerator.hpp"

#include "TriangleEntityBuilder.hpp"
#include "collision/shapes/Triangle.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "environment/EnvironmentState.hpp"
#include "environment/GridHelper.hpp"
#include "environment/HeightMapGenerator.hpp"
#include "environment/TerrainConfig.hpp"
#include "rendering/ChunkModel.hpp"
#include "rendering/ComputeShader.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/Model.hpp"
#include "rendering/Texture.hpp"
#include "rendering/TextureManager.hpp"

namespace TerrainMeshGenerator {

uint32_t currentSeed = 0;

void generateSeed() {
    currentSeed = TerrainConfig::seed;
}

void init() {
    generateSeed();
    PerlinNoise::setSeed(currentSeed);
}

TerrainMeshData generateMeshData(int gridSize, int minX, int maxX, int minZ, int maxZ,
                                 glm::ivec2 gridCoords, bool buildCollision) {
    TerrainMeshData meshData;
    meshData.minX = minX;
    meshData.maxX = maxX;

    meshData.minZ = minZ;
    meshData.maxZ = maxZ;
    meshData.gridCoords = gridCoords;
    meshData.buildCollision = buildCollision;

    generateHeightMap(meshData, minX, minZ, maxX, maxZ);

    std::vector<std::vector<glm::vec3>> vertices =
        generateVertices(minX, maxX, minZ, maxZ, meshData.heightMap);

    meshData.meshVertices_lod0 = vertices[0];
    meshData.meshVertices_lod1 = vertices[1];
    meshData.meshVertices_lod2 = vertices[2];

    // Render-only chunks skip triangle generation entirely -- collision is promoted later from
    // the baked heightmap if the camera gets close (see ChunkManager::promoteChunkCollision).
    if (buildCollision) {
        meshData.triangles = generateTerrainTriangles(vertices[0]);
    }

    return meshData;
}

std::vector<std::vector<glm::vec3>> generateVertices(int minX, int maxX, int minZ, int maxZ,
                                                     const std::vector<float>& heightMap) {
    std::vector<std::vector<glm::vec3>> vertices(3);
    int heightMapWidth = TerrainConfig::triangleCount + 3;
    auto heightAt = [&](int x, int z) { return heightMap[z * heightMapWidth + x]; };

    // Chunk-border vertices off the LOD2 lattice snap onto the chord between their multiple-of-4
    // neighbors, so every LOD tier renders the identical border polyline -- without this, a finer
    // chunk's true edge heights tear away from a coarser neighbor's chords (the per-boundary seam).
    // heightMap indices carry a 1-sample border: index 0 == grid coord -1.
    auto lockedBorderHeight = [&](int x, int z) -> float {
        int tc = TerrainConfig::triangleCount;
        if ((x == 0 || x == tc) && z % 4 != 0) {
            int z0 = (z / 4) * 4;
            float t = (z - z0) / 4.0f;
            return glm::mix(heightAt(x + 1, z0 + 1), heightAt(x + 1, z0 + 5), t);
        }
        if ((z == 0 || z == tc) && x % 4 != 0) {
            int x0 = (x / 4) * 4;
            float t = (x - x0) / 4.0f;
            return glm::mix(heightAt(x0 + 1, z + 1), heightAt(x0 + 5, z + 1), t);
        }
        return heightAt(x + 1, z + 1);
    };

    for (int z = 0; z <= TerrainConfig::triangleCount; z++) {
        for (int x = 0; x <= TerrainConfig::triangleCount; x++) {
            float t_x = static_cast<float>(x) / TerrainConfig::triangleCount;
            float t_z = static_cast<float>(z) / TerrainConfig::triangleCount;
            float posX = minX + t_x * (maxX - minX);
            float posZ = minZ + t_z * (maxZ - minZ);
            float height = lockedBorderHeight(x, z);

            glm::vec3 vertex = glm::vec3(posX, height, posZ);
            if (x % 1 == 0 && z % 1 == 0) {
                vertices[0].emplace_back(vertex);
            }
            if (x % 2 == 0 && z % 2 == 0) {
                vertices[1].emplace_back(vertex);
            }
            if (x % 4 == 0 && z % 4 == 0) {
                vertices[2].emplace_back(vertex);
            }
        }
    }

    return vertices;
}

// Post-bake filter passes. Both filters reach 1 cell per pass, so the halo must cover their
// combined reach (+1 for the final pass) or chunk borders diverge between neighbors -- that was
// the seam/hole bug. Raising iterations grows the padded grid, i.e. per-chunk noise cost.
constexpr int thermalErosionIterations = 100;
constexpr int smoothingIterations = 1;
constexpr float smoothingLambda = 0.3f;
constexpr int filterHalo = thermalErosionIterations + smoothingIterations + 1;

void generateHeightMap(TerrainMeshData& data, int minX, int minZ, int maxX, int maxZ) {
    int width = TerrainConfig::triangleCount + 3;
    data.heightMap.resize(width * width);

    if (PerlinNoise::seed != TerrainConfig::seed) {
        PerlinNoise::setSeed(TerrainConfig::seed);
    }

    // Bake into a halo-padded grid so erosion/smoothing see the same neighborhood a neighboring
    // chunk sees for the shared border samples -- the kept interior then matches bit-for-bit.
    int paddedSize = width + 2 * filterHalo;
    std::vector<float> padded(paddedSize * paddedSize);
    auto paddedAt = [&](int x, int z) -> float& { return padded[z * paddedSize + x]; };

    for (int z = -1 - filterHalo; z <= TerrainConfig::triangleCount + 1 + filterHalo; z++) {
        for (int x = -1 - filterHalo; x <= TerrainConfig::triangleCount + 1 + filterHalo; x++) {
            float t_x = static_cast<float>(x) / TerrainConfig::triangleCount;
            float t_z = static_cast<float>(z) / TerrainConfig::triangleCount;
            float posX = minX + t_x * (maxX - minX);
            float posZ = minZ + t_z * (maxZ - minZ);
            paddedAt(x + 1 + filterHalo, z + 1 + filterHalo) =
                HeightMapGenerator::getHeightAt(posX, posZ);
        }
    }

    HeightMapGenerator::thermalErode(padded, paddedSize, thermalErosionIterations);
    HeightMapGenerator::laplacianSmooth(padded, paddedSize, smoothingIterations, smoothingLambda);

    // Crop the interior back out; cells within the halo of the padded edge are contaminated by
    // the frozen boundary and discarded.
    for (int z = 0; z < width; z++) {
        for (int x = 0; x < width; x++) {
            float height = paddedAt(x + filterHalo, z + filterHalo);
            data.heightMap[z * width + x] = height;
            if (height < data.minHeight) data.minHeight = height;
            if (height > data.maxHeight) data.maxHeight = height;
            if (height < EnvironmentState::globalMinHeight)
                EnvironmentState::globalMinHeight = height;
            if (height > EnvironmentState::globalMaxHeight)
                EnvironmentState::globalMaxHeight = height;
        }
    }

    // One extra texel of halo beyond heightMap's own border, so calculateNormals's Sobel kernel
    // has real (bit-for-bit-consistent, since it comes from the same padded bake) neighbor data
    // for every texel it produces -- without this, normals at a chunk's outer edge would either
    // sample out of bounds or fall back to a duplicated/clamped edge value, producing a visible
    // normal seam against the neighboring chunk's independently-baked edge. kFilterHalo (47) is
    // far larger than the 1 extra texel needed here, so this crop is effectively free.
    int normalSourceWidth = width + 2;
    std::vector<float> normalSourceHeightMap(normalSourceWidth * normalSourceWidth);
    for (int z = 0; z < normalSourceWidth; z++) {
        for (int x = 0; x < normalSourceWidth; x++) {
            normalSourceHeightMap[z * normalSourceWidth + x] =
                paddedAt(x - 1 + filterHalo, z - 1 + filterHalo);
        }
    }

    float worldTexelSize = static_cast<float>(maxX - minX) / TerrainConfig::triangleCount;
    data.normalMap = calculateNormals(normalSourceHeightMap, normalSourceWidth, worldTexelSize);
}

std::vector<glm::vec2> calculateNormals(const std::vector<float>& heightMap, int width,
                                        float worldTexelSize) {
    int outWidth = width - 2;
    std::vector<glm::vec2> normals(outWidth * outWidth);
    auto heightAt = [&](int x, int z) { return heightMap[z * width + x]; };

    // Identical formula to terrain.tese/gbufferTerrain.tese's Sobel gradient -- see those files
    // for the derivation. Must stay in sync.
    float scale = 1.0f / (8.0f * worldTexelSize);
    for (int z = 0; z < outWidth; z++) {
        for (int x = 0; x < outWidth; x++) {
            int cx = x + 1, cz = z + 1;  // center in heightMap's padded coordinates
            float h00 = heightAt(cx - 1, cz - 1);
            float h10 = heightAt(cx, cz - 1);
            float h20 = heightAt(cx + 1, cz - 1);
            float h01 = heightAt(cx - 1, cz);
            float h21 = heightAt(cx + 1, cz);
            float h02 = heightAt(cx - 1, cz + 1);
            float h12 = heightAt(cx, cz + 1);
            float h22 = heightAt(cx + 1, cz + 1);

            float Gx = (h20 + 2.0f * h21 + h22) - (h00 + 2.0f * h01 + h02);
            float Gz = (h02 + 2.0f * h12 + h22) - (h00 + 2.0f * h10 + h20);

            glm::vec3 normal = glm::normalize(glm::vec3(-Gx * scale, 1.0f, -Gz * scale));
            normals[z * outWidth + x] = glm::vec2(normal.x, normal.z);
        }
    }
    return normals;
}

std::vector<GLuint> generateHeightMapTextures(const std::vector<float>& heightMap) {
    GLuint heightMapTexture;

    int heightMapSize = TerrainConfig::triangleCount + 3;
    int normalMapSize = TerrainConfig::triangleCount + 1;

    glGenTextures(1, &heightMapTexture);

    glBindTexture(GL_TEXTURE_2D, heightMapTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, heightMapSize, heightMapSize, 0, GL_RED, GL_FLOAT,
                 heightMap.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint normalMapTexture;
    glGenTextures(1, &normalMapTexture);

    glBindTexture(GL_TEXTURE_2D, normalMapTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, normalMapSize, normalMapSize, 0, GL_RGBA, GL_FLOAT,
                 nullptr);

    // Set texture wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // No mipmaps: the compute shader only ever writes mip level 0, so generating mips here (before
    // that write happens) would just bake in stale/garbage data at every level above 0.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLuint tangentMapTexture;
    glGenTextures(1, &tangentMapTexture);
    glBindTexture(GL_TEXTURE_2D, tangentMapTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, normalMapSize, normalMapSize, 0, GL_RGBA, GL_FLOAT,
                 nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint bitangentMapTexture;
    glGenTextures(1, &bitangentMapTexture);
    glBindTexture(GL_TEXTURE_2D, bitangentMapTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, normalMapSize, normalMapSize, 0, GL_RGBA, GL_FLOAT,
                 nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    ComputeShader* compute = Display.getNormalMapShader();

    compute->use();

    compute->setFloat("heightScale", 1.0f);
    compute->setFloat("chunkWorldSize", static_cast<float>(TerrainConfig::gridSize));
    compute->setInt("heightfield", 0);

    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, heightMapTexture);

    glBindImageTexture(1, normalMapTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(2, tangentMapTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(3, bitangentMapTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    const int WORK_GROUP_SIZE = 2;
    int groupsX = (normalMapSize + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
    int groupsY = (normalMapSize + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;

    compute->dispatch(groupsX, groupsY);

    glDeleteTextures(1, &heightMapTexture);

    return std::vector<GLuint>{normalMapTexture, tangentMapTexture, bitangentMapTexture};
}

std::vector<TriangleEntityBuilder> generateTerrainTriangles(
    const std::vector<glm::vec3>& vertices) {
    // Grid of [2 * numRows][numCols] where each pair of rows corresponds to the two triangles per
    // quad row
    // Could be expensive to pre-allocate all these entt::null values, consider a flatenedt array

    std::vector<TriangleEntityBuilder> triangles;

    // triangleGrid contains 2 rows per single row of triangles. Each pair represents a single quad
    // from triangles.

    const float DEGENERATE_AREA_EPS = 0.00005f;  // tune based on your terrain scale

    int rowWidth = TerrainConfig::triangleCount + 1;

    for (int z = 0; z < TerrainConfig::triangleCount; z++) {
        for (int x = 0; x < TerrainConfig::triangleCount; x++) {
            // First triangle of the quad

            int topLeft = z * rowWidth + x;
            int topRight = z * rowWidth + (x + 1);
            int bottomLeft = (z + 1) * rowWidth + x;
            int bottomRight = (z + 1) * rowWidth + (x + 1);

            glm::vec3 v1 = vertices[topLeft];
            glm::vec3 v2 = vertices[topRight];
            glm::vec3 v3 = vertices[bottomLeft];

            glm::vec3 edge1 = v2 - v1;
            glm::vec3 edge2 = v3 - v1;
            float area = 0.5f * glm::length(glm::cross(edge1, edge2));
            if (area < DEGENERATE_AREA_EPS) {
                continue;
            } else {
                TriangleEntityBuilder entity;
                // Vertex order swapped (v3, v2 instead of v2, v3) so cross(edge1, edge2) in
                // Triangle::defineBoundingVolume yields an upward-facing normal for this
                // grid winding — topLeft->topRight->bottomLeft is CW from above and produces
                // -Y otherwise.
                entity.addData(v1, v3, v2, topLeft, bottomLeft, topRight);
                triangles.push_back(entity);
            }

            // Second triangle of the quad

            v1 = vertices[bottomLeft];
            v2 = vertices[topRight];
            v3 = vertices[bottomRight];
            edge1 = v2 - v1;
            edge2 = v3 - v1;
            area = 0.5f * glm::length(glm::cross(edge1, edge2));
            if (area < DEGENERATE_AREA_EPS) {
                continue;
            } else {
                TriangleEntityBuilder entity;
                // Same winding fix as the first triangle above.
                entity.addData(v1, v3, v2, bottomLeft, bottomRight, topRight);
                triangles.push_back(entity);
            }
        }
    }

    // Clean up the temporary grid
    return triangles;
}

std::vector<unsigned int> generateStripIndices() {
    std::vector<unsigned int> stripIndices;

    int rowVertices = TerrainConfig::triangleCount + 1;

    for (int z = 0; z < TerrainConfig::triangleCount; ++z) {
        // Add the row strip
        for (int x = 0; x <= TerrainConfig::triangleCount; ++x) {
            stripIndices.push_back((z + 0) * rowVertices + x);  // top row
            stripIndices.push_back((z + 1) * rowVertices + x);  // bottom row
        }

        // Add two degenerate triangles (if not the last row)
        if (z < TerrainConfig::triangleCount - 1) {
            stripIndices.push_back(
                (z + 1) * rowVertices +
                TerrainConfig::triangleCount);  // repeat last vertex of current strip
            stripIndices.push_back((z + 1) * rowVertices + 0);  // first vertex of next strip
        }
    }

    return std::move(stripIndices);
}

std::vector<unsigned int> generateQuadIndices() {
    std::vector<unsigned int> quadIndices;

    for (int z = 0; z < TerrainConfig::triangleCount; z++) {
        for (int x = 0; x < TerrainConfig::triangleCount; x++) {
            unsigned int topLeft = z * (TerrainConfig::triangleCount + 1) + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * (TerrainConfig::triangleCount + 1) + x;
            unsigned int bottomRight = bottomLeft + 1;

            // First triangle
            quadIndices.push_back(topLeft);
            quadIndices.push_back(bottomLeft);
            quadIndices.push_back(topRight);

            // Second triangle
            quadIndices.push_back(topRight);
            quadIndices.push_back(bottomLeft);
            quadIndices.push_back(bottomRight);
        }
    }

    return quadIndices;
}

std::vector<glm::vec2> generateNormalizedUVs(const std::vector<glm::vec3>& vertices) {
    std::vector<glm::vec2> uvCoordinates;
    uvCoordinates.reserve(vertices.size());

    for (int z = 0; z <= TerrainConfig::triangleCount; z++) {
        for (int x = 0; x <= TerrainConfig::triangleCount; x++) {
            float u = (float)x / (float)TerrainConfig::triangleCount;  // 0.0 to 1.0
            float v = (float)z / (float)TerrainConfig::triangleCount;  // 0.0 to 1.0
            uvCoordinates.push_back(glm::vec2(u, v));
        }
    }

    return uvCoordinates;
}

}  // namespace TerrainMeshGenerator