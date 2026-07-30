#pragma once
#include <chrono>
#include <deque>
#include <future>
#include <limits>
#include <unordered_set>
#include <vector>

#include "TerrainConfig.hpp"
#include "TriangleEntityBuilder.hpp"
#include "common/PairHash.hpp"
#include "environment/CollisionShapeGrid.hpp"
#include "environment/GridHelper.hpp"
#include "environment/TerrainAssetFactory.hpp"
#include "rendering/ChunkModel.hpp"

struct TerrainMeshData;
class Chunk;

struct ChunkTask {
    glm::ivec2 gridCoords = glm::ivec2(0);
    bool isGenerated = false;
    bool buildCollision = true;

    ChunkTask(glm::ivec2 gridCoords, bool buildCollision = true)
        : gridCoords(gridCoords), buildCollision(buildCollision) {}
};

struct TriangleTask {
    glm::ivec2 gridCoords = glm::ivec2(0);
    std::vector<TriangleEntityBuilder> triangles;
    std::vector<uint32_t> triangleIDs;
    uint32_t chunkSlot = std::numeric_limits<uint32_t>::max();
    int completedTriangles = 0;
    int totalTriangles = 0;
    bool isGenerated = false;

    TriangleTask() = default;
    TriangleTask(glm::ivec2 gridCoords, std::vector<TriangleEntityBuilder> triangleData)
        : gridCoords(gridCoords),
          triangles(triangleData),
          totalTriangles(static_cast<int>(triangles.size())) {}
};

class ChunkGenerationPipeline {
   public:
    ChunkGenerationPipeline(std::unordered_map<glm::ivec2, std::unique_ptr<CollisionShapeGrid>,
                                               PairHash_1>& chunkCollisionGrid,
                            std::unordered_set<glm::ivec2, PairHash_1>& loadedChunks,
                            ChunkModel& chunkModel, int maxChunksPerIteration = 10);
    ~ChunkGenerationPipeline();

    TerrainAssetFactory terrainAssetFactory;

    void init();
    void addChunkCreationTask(const ChunkTask& task);
    void createChunk(TerrainMeshData& meshData);
    void processAttributeGenerationQueue();
    void completeTriangleTasks(glm::ivec2 gridCoords, TriangleTask& task,
                               std::chrono::steady_clock::time_point deadline);
    void completeAttributeGenerationQueue();
    void runIfNeeded();
    bool chunkExists(const glm::ivec2& gridCoords) const;
    void updateWorldBounds(float chunkMinX, float chunkMinZ, float chunkMaxX, float chunkMaxZ);

    TerrainMeshData loadChunkAtPosition(const glm::ivec2& gridCoords, bool buildCollision);

    void runPostProcessing();
    // (Re)bakes the world normal/tangent/bitangent maps for the current world bounds, sampling
    // each texel from the baked per-chunk heightmap where loaded (falls back to raw noise for
    // chunks outside the loaded window) -- coarse resolution, shading-only purpose.
    // Safe to call repeatedly as the window moves; frees the previous bake's GL resources first.

    std::deque<ChunkTask> creationTaskQueue;

    std::deque<ChunkTask> attributeGenerationQueue;

    std::vector<std::future<TerrainMeshData>> attributeGenerationFutures;
    std::unordered_set<glm::ivec2, PairHash_1> processingChunks;
    std::unordered_map<glm::ivec2, TriangleTask, PairHash_1> triangleGenerationTasks;
    std::unordered_map<glm::ivec2, std::unique_ptr<CollisionShapeGrid>, PairHash_1>&
        chunkCollisionGrid;
    std::unordered_set<glm::ivec2, PairHash_1>& loadedChunks;

    ChunkModel& chunkModel;

    bool isLoading = false;
    bool isLoaded = false;
    int totalChunksLoaded;
    int totalChunksLoading;
    float lookUpRadius = TerrainConfig::gridSize;

    int maxChunksPerIteration;
    int currentChunkIteration;

   private:
    // Previous bake's GL ids + wrappers (Texture doesn't own its GL id), freed on each rebake.
    std::vector<GLuint> generatedWorldMapIds;
    std::vector<Texture*> generatedWorldMapTextures;
};
