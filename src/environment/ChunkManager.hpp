#pragma once
#include "ChunkGenerationPipeline.hpp"
#include "GridHelper.hpp"
#include "TerrainConfig.hpp"
#include "core/PrecompiledHeader.hpp"
#include "environment/TerrainMeshGeneratorStructs.hpp"

class Triangle;
class StaticTriangleSource;
class TriangleGrid;

class ChunkManager {
   public:
    ChunkManager() {}
    ~ChunkManager() = default;
    uint32_t currentSeed;
    void update();
    void init();
    // Tears down and rebuilds every chunk/collision/GL structure sized off
    // TerrainConfig::triangleCount, then reprimes the world around the camera. Called when
    // Triangle Count is changed live from the debug panel's Terrain tab.
    void regenerateTerrain();
    void lookAhead(int lookUpRadius);
    // Rebuilds LOD0/1 meshes + collision triangles for a render-only chunk from its baked
    // heightmap when the camera enters the collision window; no noise re-evaluation involved.
    void promoteChunkCollision(const glm::ivec2& gridCoords);
    void destructGrids();
    bool chunkExistsDeployed(const glm::ivec2& gridCoords) const;

    uint32_t hash(uint32_t x);

    float getLoadingProgress() {
        if (!genPipeline.isLoaded && !genPipeline.isLoading) return 0.f;
        int total = genPipeline.totalChunksLoading;
        int loaded = genPipeline.totalChunksLoaded;
        if (total == 0) {
            return 1.0f;  // nothing to load â†’ consider progress complete
        }
        float progress = (float)loaded / (float)total;
        return std::clamp(progress, 0.0f, 1.0f);
    }

    bool getIsLoading() { return genPipeline.isLoading; }

    void startIfNeeded() {
        if (!genPipeline.isLoaded && !genPipeline.isLoading) {
            init();
        }
    }

    bool isLoaded = false;
    bool isLoading = false;
    int totalChunksLoading = 0;
    int totalChunksLoaded = 0;

    glm::vec2 currentGridMax;
    glm::vec2 currentGridMin;

    CollisionShapeGrid* getTerrainAt(const glm::vec3& worldPosition);

    // Bilerps the owning chunk's baked heightmap; returns false when that chunk isn't loaded.
    bool sampleBakedHeight(float x, float z, float& outHeight) const;
    // Central-difference normal over the baked heightmaps; flat up if a neighbor sample is missing.
    glm::vec3 sampleBakedNormal(float x, float z) const;

    std::vector<glm::vec3>& getChunkVertices() { return chunkVertices; }

    ChunkModel chunkModel;
    ChunkModel& getChunkModel() { return chunkModel; }

    std::unordered_map<glm::ivec2, std::unique_ptr<CollisionShapeGrid>, PairHash_1>
        chunkCollisionGrid;
    std::unordered_set<glm::ivec2, PairHash_1> loadedChunks;
    std::vector<glm::vec3> chunkVertices;
    ChunkGenerationPipeline genPipeline =
        ChunkGenerationPipeline(chunkCollisionGrid, loadedChunks, chunkModel);
};
