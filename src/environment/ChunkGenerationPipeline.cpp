#include "ChunkGenerationPipeline.hpp"

#include <chrono>
#include <glm/ext/vector_int2.hpp>
#include <memory>
#include <thread>
#include <tracy/Tracy.hpp>

#include "HeightMapGenerator.hpp"
#include "TerrainMeshGenerator.hpp"
#include "TerrainMeshGeneratorStructs.hpp"
#include "components/physics/KinematicBody.hpp"
#include "core/Core.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "environment/GridHelper.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/DebugState.hpp"
#include "rendering/InstancedModel.hpp"
#include "rendering/SceneState.hpp"
#include "rendering/Texture.hpp"
#include "rendering/TextureManager.hpp"

ChunkGenerationPipeline::ChunkGenerationPipeline(
    std::unordered_map<glm::ivec2, std::unique_ptr<CollisionShapeGrid>, PairHash_1>&
        chunkCollisionGrid,
    std::unordered_set<glm::ivec2, PairHash_1>& loadedChunks, ChunkModel& chunkModel,
    int maxChunksPerIteration)
    : chunkCollisionGrid(chunkCollisionGrid),
      loadedChunks(loadedChunks),
      chunkModel(chunkModel),
      isLoading(false),
      totalChunksLoaded(0),
      totalChunksLoading(0),
      maxChunksPerIteration(maxChunksPerIteration),
      currentChunkIteration(0) {
    ZoneScoped;
    TerrainMeshGenerator::init();
}

ChunkGenerationPipeline::~ChunkGenerationPipeline() = default;

void ChunkGenerationPipeline::runIfNeeded() {
    ZoneScoped;
    if (attributeGenerationQueue.empty() && attributeGenerationFutures.empty()) return;

    using clock = std::chrono::steady_clock;

    processAttributeGenerationQueue();

    completeAttributeGenerationQueue();

    KinematicBody& cameraBody = Registry.get<KinematicBody>(CameraController::activeCamera);
    glm::ivec2 gridCoords = GridHelper::worldToChunkIndex(cameraBody.position);

    auto afterCameraLookup = clock::now();

    // Don't throttle triangle generation during the initial load - only once the world is fully
    // loaded and streaming is happening behind gameplay do we need to stay off the main thread's
    // budget.
    auto triangleBudgetDeadline =
        isLoaded ? afterCameraLookup + std::chrono::microseconds(2500) : clock::time_point::max();

    for (auto it = triangleGenerationTasks.begin();
         it != triangleGenerationTasks.end() && clock::now() < triangleBudgetDeadline;) {
        TriangleTask& task = it->second;
        glm::ivec2 gridCoord = task.gridCoords;
        completeTriangleTasks(gridCoord, task, triangleBudgetDeadline);
        if (task.isGenerated) {
            auto triangleIDs = std::move(task.triangleIDs);
            it = triangleGenerationTasks.erase(it);
            for (auto& triangle : triangleIDs) {
                chunkCollisionGrid[gridCoord]->insertObject(triangle);
            }
        } else
            it++;
    }
}

void ChunkGenerationPipeline::addChunkCreationTask(const ChunkTask& task) {
    ZoneScoped;
    attributeGenerationQueue.push_back(task);
    processingChunks.insert(task.gridCoords);

    if (isLoading) {
        totalChunksLoading++;
    }
}

void ChunkGenerationPipeline::completeTriangleTasks(
    glm::ivec2 gridCoords, TriangleTask& task, std::chrono::steady_clock::time_point deadline) {
    ZoneScoped;
    if (task.chunkSlot == std::numeric_limits<uint32_t>::max()) {
        task.chunkSlot = Triangles.allocateChunk(gridCoords);
    }

    for (int i = task.completedTriangles; i < task.totalTriangles; i++) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return;
        }
        auto triangleID = task.triangles[i].buildEntity(Triangles, size_t(task.chunkSlot));
        task.triangleIDs.push_back(triangleID);
        task.completedTriangles++;
    }

    task.isGenerated = true;
}

void ChunkGenerationPipeline::createChunk(TerrainMeshData& meshData) {
    ZoneScoped;

    glm::vec3 aabbMin(meshData.minX, meshData.minHeight, meshData.minZ);
    glm::vec3 aabbMax(meshData.maxX, meshData.maxHeight, meshData.maxZ);

    if (meshData.buildCollision) {
        triangleGenerationTasks[meshData.gridCoords] =
            TriangleTask(meshData.gridCoords, meshData.triangles);

        chunkModel.loadChunk(meshData.meshVertices_lod0, aabbMin, aabbMax, 0);
        chunkModel.loadChunk(meshData.meshVertices_lod1, aabbMin, aabbMax, 1);
        chunkModel.loadChunk(meshData.meshVertices_lod2, aabbMin, aabbMax, 2);

        chunkCollisionGrid[meshData.gridCoords] = std::make_unique<CollisionShapeGrid>(
            glm::vec2(meshData.minX, meshData.minZ), glm::vec2(meshData.maxX, meshData.maxZ),
            static_cast<int>(meshData.triangles.size()));
    } else {
        // Render-only chunk: LOD2 mesh only, flagged to draw at any LOD distance until promoted.
        chunkModel.loadChunk(meshData.meshVertices_lod2, aabbMin, aabbMax, 2, true);
    }

    loadedChunks.insert(meshData.gridCoords);

    // heightMap/normalMap indices carry a 1-sample border (index 0 == grid coord -1, see
    // TerrainMeshGenerator::generateHeightMap), so the chunk's block starts one texel before its
    // own origin in the clipmap's global texel space -- adjacent chunks' blocks then overlap by a
    // few texels at the shared border, which is fine since the halo-padded bake guarantees those
    // overlapping samples match bit-for-bit between neighbors.
    int heightMapWidth = TerrainConfig::triangleCount + 3;
    chunkModel.normalMapBuffer.upload(meshData.gridCoords.x * TerrainConfig::triangleCount - 1,
                                      meshData.gridCoords.y * TerrainConfig::triangleCount - 1,
                                      heightMapWidth, heightMapWidth, meshData.normalMap);

    chunkModel.heightMaps[meshData.gridCoords] = std::move(meshData.heightMap);
}

void ChunkGenerationPipeline::processAttributeGenerationQueue() {
    ZoneScoped;
    if (attributeGenerationQueue.empty()) return;

    KinematicBody& cameraBody = Registry.get<KinematicBody>(CameraController::activeCamera);
    glm::ivec2 center = GridHelper::worldToChunkIndex(cameraBody.position);
    auto chebyshev = [&](const glm::ivec2& coords) {
        return std::max(std::abs(coords.x - center.x), std::abs(coords.y - center.y));
    };

    // Purge queued (never dispatched) tasks the camera has left behind; in-flight futures can't
    // be cancelled, but the concurrency cap below keeps almost everything in this queue
    // instead.
    for (auto it = attributeGenerationQueue.begin(); it != attributeGenerationQueue.end();) {
        if (chebyshev(it->gridCoords) > TerrainConfig::renderRadius + 1) {
            processingChunks.erase(it->gridCoords);
            if (isLoading) totalChunksLoading--;
            it = attributeGenerationQueue.erase(it);
        } else {
            ++it;
        }
    }

    // Nearest-first: the chunk you're about to reach always builds before the horizon does.
    std::sort(attributeGenerationQueue.begin(), attributeGenerationQueue.end(),
              [&](const ChunkTask& a, const ChunkTask& b) {
                  return chebyshev(a.gridCoords) < chebyshev(b.gridCoords);
              });

    // Fill workers only up to the core count so the backlog stays reorderable/discardable here
    // rather than being buried in uncancellable std::async futures.
    size_t maxInFlight = std::max(4u, std::thread::hardware_concurrency());
    while (!attributeGenerationQueue.empty() && attributeGenerationFutures.size() < maxInFlight) {
        ChunkTask task = attributeGenerationQueue.front();
        attributeGenerationQueue.pop_front();

        attributeGenerationFutures.emplace_back(std::async(std::launch::async, [this, task] {
            return loadChunkAtPosition(task.gridCoords, task.buildCollision);
        }));
    }
}

void ChunkGenerationPipeline::runPostProcessing() {
    ZoneScoped;

    if (DebugState::shouldGenerateFoliage) {
        terrainAssetFactory.generateTreePlacements(chunkCollisionGrid);
    }
}

void ChunkGenerationPipeline::completeAttributeGenerationQueue() {
    ZoneScoped;
    ZoneValue(attributeGenerationFutures.size());
    currentChunkIteration = 0;

    KinematicBody& cameraBody = Registry.get<KinematicBody>(CameraController::activeCamera);
    glm::ivec2 center = GridHelper::worldToChunkIndex(cameraBody.position);

    for (auto it = attributeGenerationFutures.begin(); it != attributeGenerationFutures.end();) {
        if (currentChunkIteration == maxChunksPerIteration) return;
        TerrainMeshData meshData;
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            meshData = it->get();
            // Stale completion: the camera moved on while this generated. Drop it instead of
            // uploading a chunk destructGrids would immediately tear back down. During the
            // initial load the camera is static, and skipping would stall the loaded==loading
            // completion gate, so everything counts there.
            int chebyshev = std::max(std::abs(meshData.gridCoords.x - center.x),
                                     std::abs(meshData.gridCoords.y - center.y));
            if (!isLoading && chebyshev > TerrainConfig::renderRadius + 1) {
                processingChunks.erase(meshData.gridCoords);
                it = attributeGenerationFutures.erase(it);
                continue;
            }
            createChunk(meshData);
            it = attributeGenerationFutures.erase(it);
            processingChunks.erase(meshData.gridCoords);

            if (isLoading) {
                totalChunksLoaded++;
                if (totalChunksLoaded == totalChunksLoading) {
                    runPostProcessing();
                    isLoading = false;
                    isLoaded = true;
                }
            }

            currentChunkIteration++;
        } else {
            ++it;
        }
    }
    ZoneValue(currentChunkIteration);
}

TerrainMeshData ChunkGenerationPipeline::loadChunkAtPosition(const glm::ivec2& gridCoords,
                                                             bool buildCollision) {
    ZoneScoped;
    int minX = gridCoords.x * TerrainConfig::gridSize;
    int maxX = minX + TerrainConfig::gridSize;
    int minZ = gridCoords.y * TerrainConfig::gridSize;
    int maxZ = minZ + TerrainConfig::gridSize;

    updateWorldBounds(minX, minZ, maxX, maxZ);

    using clock = std::chrono::steady_clock;
    auto stepStart = clock::now();

    TerrainMeshData meshData = TerrainMeshGenerator::generateMeshData(
        TerrainConfig::gridSize, minX, maxX, minZ, maxZ, gridCoords, buildCollision);

    // auto afterGenerate = clock::now();
    // std::cout << "[PROFILE] vertexGeneration (" << gridCoords.x << ", " << gridCoords.y << ")
    // "
    //           << std::chrono::duration<double, std::milli>(afterGenerate - stepStart).count()
    //           << "ms" << std::endl;

    return meshData;
}

bool ChunkGenerationPipeline::chunkExists(const glm::ivec2& gridCoords) const {
    ZoneScoped;
    for (const auto& slot : chunkModel.slots) {
        if ((slot.active && slot.aabbMin.x <= gridCoords.x * TerrainConfig::gridSize &&
             slot.aabbMax.x >= gridCoords.x * TerrainConfig::gridSize &&
             slot.aabbMin.z <= gridCoords.y * TerrainConfig::gridSize &&
             slot.aabbMax.z >= gridCoords.y * TerrainConfig::gridSize) ||
            (processingChunks.find(gridCoords) != processingChunks.end())) {
            return true;
        }
    }
    return false;
}

void ChunkGenerationPipeline::updateWorldBounds(float chunkMinX, float chunkMinZ, float chunkMaxX,
                                                float chunkMaxZ) {
    ZoneScoped;
    if (chunkMinX < EnvironmentState::worldMin.x) EnvironmentState::worldMin.x = chunkMinX;
    if (chunkMinZ < EnvironmentState::worldMin.y) EnvironmentState::worldMin.y = chunkMinZ;

    if (chunkMaxX > EnvironmentState::worldMax.x) EnvironmentState::worldMax.x = chunkMaxX;
    if (chunkMaxZ > EnvironmentState::worldMax.y) EnvironmentState::worldMax.y = chunkMaxZ;
}