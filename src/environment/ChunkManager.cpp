#include "ChunkManager.hpp"

#include <tracy/Tracy.hpp>

#include "components/physics/KinematicBody.hpp"
#include "components/rendering/ChunkRenderable.hpp"
#include "core/Core.hpp"
#include "core/Game.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "environment/EnvironmentState.hpp"
#include "environment/HeightMapGenerator.hpp"
#include "environment/TerrainMeshGenerator.hpp"
#include "environment/TriangleManager.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/DisplayManager.hpp"

bool ChunkManager::sampleBakedHeight(float x, float z, float& outHeight) const {
    glm::ivec2 gridCoords = GridHelper::worldToChunkIndex(glm::vec3(x, 0.0f, z));
    auto it = chunkModel.heightMaps.find(gridCoords);
    if (it == chunkModel.heightMaps.end()) return false;

    outHeight = HeightMapGenerator::bilerpHeightAt(it->second, TerrainConfig::triangleCount + 3, x,
                                                   z, gridCoords.x * TerrainConfig::gridSize,
                                                   gridCoords.y * TerrainConfig::gridSize);
    return true;
}

glm::vec3 ChunkManager::sampleBakedNormal(float x, float z) const {
    float spacing = static_cast<float>(TerrainConfig::gridSize) / TerrainConfig::triangleCount;

    float hL, hR, hD, hU;
    if (!sampleBakedHeight(x - spacing, z, hL) || !sampleBakedHeight(x + spacing, z, hR) ||
        !sampleBakedHeight(x, z - spacing, hD) || !sampleBakedHeight(x, z + spacing, hU)) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    float dhdx = (hR - hL) / (2.0f * spacing);
    float dhdz = (hU - hD) / (2.0f * spacing);
    return glm::normalize(glm::vec3(-dhdx, 1.0f, -dhdz));
}

uint32_t ChunkManager::hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;
    return x ^ 0x7fffffff;
}

void ChunkManager::init() {
    // Collision capacity covers destructGrids()'s 2 * lookUpRadius hysteresis window; render
    // capacity covers the wide renderRadius window (+2 rings of destruct hysteresis).
    chunkModel.initialize(TerrainConfig::collisionCapacity, TerrainConfig::renderCapacity);
    Triangles.initialize(TerrainConfig::collisionCapacity);

    auto e = Registry.create();
    Registry.emplace<ChunkRenderable>(e, &chunkModel, Display.getTerrainShaderID(),
                                      RenderLayer::Terrain, true);
    if (genPipeline.isLoaded) return;

    genPipeline.isLoaded = false;
    genPipeline.isLoading = true;

    // Prime the full render window up front so the horizon is populated when the world opens.
    lookAhead(TerrainConfig::renderRadius);
    std::cout << "Completed initial lookAhead" << std::endl;
}

void ChunkManager::regenerateTerrain() {
    // chunkCollisionGrid/Triangles are read every tick by the physics/collision FixedRateClock
    // threads (see CLAUDE.md's architecture notes) -- pause them before tearing that state down
    // on the main thread so a callback mid-tick can't race the reset. stop() joins the ticker
    // thread, so nothing is still in flight once this returns.
    if (Game::clock120) Game::clock120->stop();
    if (Game::clock240) Game::clock240->stop();

    for (size_t i = 0; i < chunkModel.maxLoadedChunks; i++) {
        if (chunkModel.slots[i].active) chunkModel.unloadChunk(static_cast<int>(i));
    }
    chunkModel.heightMaps.clear();
    chunkCollisionGrid.clear();
    loadedChunks.clear();
    chunkVertices.clear();

    genPipeline.creationTaskQueue.clear();
    genPipeline.attributeGenerationQueue.clear();
    // These are still generating on worker threads at the old triangleCount -- wait for them to
    // land rather than discarding the futures, so nothing tries to write into a std::async'd
    // TerrainMeshData after ChunkModel's buffers have already been reallocated out from under it.
    for (auto& future : genPipeline.attributeGenerationFutures) future.wait();
    genPipeline.attributeGenerationFutures.clear();
    genPipeline.processingChunks.clear();
    genPipeline.triangleGenerationTasks.clear();
    genPipeline.totalChunksLoaded = 0;
    genPipeline.totalChunksLoading = 0;
    genPipeline.isLoaded = false;
    genPipeline.isLoading = true;

    chunkModel.initialize(TerrainConfig::collisionCapacity, TerrainConfig::renderCapacity);
    Triangles.initialize(TerrainConfig::collisionCapacity);

    lookAhead(TerrainConfig::renderRadius);

    if (Game::clock120) Game::clock120->start();
    if (Game::clock240) Game::clock240->start();
}

void ChunkManager::update() {
    ZoneScoped;
    {
        ZoneScopedN("ChunkManager::update: lookAhead");
        lookAhead(TerrainConfig::renderRadius);
    }
    {
        ZoneScopedN("ChunkManager::update: destructGrids");
        destructGrids();
    }
    {
        ZoneScopedN("ChunkManager::update: genPipeline.runIfNeeded");
        genPipeline.runIfNeeded();
    }
    {
        ZoneScopedN("ChunkManager::update: world map drift check");
        // TODO: The baked normal/tangent maps go stale
        // The baked normal/tangent maps go stale (stretched over a window they weren't baked for)
        // once the camera drifts far enough from the bake center; rebake before that becomes
        // visible. GL texture upload + a coarse-resolution noise pass, not the per-chunk erosion
        // bake, so this is cheap enough to check every frame once loaded. if (genPipeline.isLoaded)
        // {
        //     CameraState& cameraState = Registry.get<CameraState>(CameraController::activeCamera);
        //     glm::ivec2 gridCoords = GridHelper::worldToChunkIndex(cameraState.cameraPosition);
        //     int drift = std::max(std::abs(gridCoords.x - genPipeline.worldMapBakeCenter.x),
        //                          std::abs(gridCoords.y - genPipeline.worldMapBakeCenter.y));
        //     if (drift >= std::max(TerrainConfig::renderRadius / 4, 1)) {
        //         genPipeline.regenerateWorldMaps();
        //     }
        // }
    }
}
void ChunkManager::lookAhead(int lookUpRadius) {
    ZoneScoped;
    KinematicBody& cameraBody = Registry.get<KinematicBody>(CameraController::activeCamera);

    glm::vec3 cameraPosition = cameraBody.position;
    glm::ivec2 initialGridCoords = GridHelper::worldToChunkIndex(cameraPosition);

    int tasksAdded = 0;
    for (int x = initialGridCoords.x - lookUpRadius; x < initialGridCoords.x + lookUpRadius; x++) {
        for (int z = initialGridCoords.y - lookUpRadius; z < initialGridCoords.y + lookUpRadius;
             z++) {
            glm::ivec2 currentGrid = glm::ivec2(x, z);

            // Chunks inside the collision window get the full build; the rest stream render-only.
            int chebyshev =
                std::max(std::abs(x - initialGridCoords.x), std::abs(z - initialGridCoords.y));
            bool needsCollision = chebyshev < TerrainConfig::lookUpRadius;

            if (genPipeline.processingChunks.find(currentGrid) !=
                genPipeline.processingChunks.end()) {
                continue;
            }

            if (loadedChunks.find(currentGrid) != loadedChunks.end()) {
                // Already loaded render-only chunk entering the collision window: promote it.
                if (needsCollision &&
                    chunkCollisionGrid.find(currentGrid) == chunkCollisionGrid.end()) {
                    promoteChunkCollision(currentGrid);
                }
                continue;
            }

            genPipeline.addChunkCreationTask(ChunkTask(currentGrid, needsCollision));
            tasksAdded++;
        }
    }
    ZoneValue(tasksAdded);
}

void ChunkManager::promoteChunkCollision(const glm::ivec2& gridCoords) {
    ZoneScoped;
    auto heightIt = chunkModel.heightMaps.find(gridCoords);
    if (heightIt == chunkModel.heightMaps.end()) return;

    int minX = gridCoords.x * TerrainConfig::gridSize;
    int maxX = minX + TerrainConfig::gridSize;
    int minZ = gridCoords.y * TerrainConfig::gridSize;
    int maxZ = minZ + TerrainConfig::gridSize;

    // The existing LOD2 slot already carries the chunk's height-aware AABB.
    int lod2Slot = chunkModel.findSlot(glm::vec3(minX, 0.0f, minZ), 2);
    if (lod2Slot < 0) return;
    glm::vec3 aabbMin = chunkModel.slots[lod2Slot].aabbMin;
    glm::vec3 aabbMax = chunkModel.slots[lod2Slot].aabbMax;

    // Why are we doing this again???
    std::vector<std::vector<glm::vec3>> vertices =
        TerrainMeshGenerator::generateVertices(minX, maxX, minZ, maxZ, heightIt->second);

    std::vector<TriangleEntityBuilder> triangles =
        TerrainMeshGenerator::generateTerrainTriangles(vertices[0]);
    chunkCollisionGrid[gridCoords] = std::make_unique<CollisionShapeGrid>(
        glm::vec2(minX, minZ), glm::vec2(maxX, maxZ), static_cast<int>(triangles.size()));
    genPipeline.triangleGenerationTasks[gridCoords] =
        TriangleTask(gridCoords, std::move(triangles));

    chunkModel.loadChunk(vertices[0], aabbMin, aabbMax, 0);
    chunkModel.loadChunk(vertices[1], aabbMin, aabbMax, 1);
    chunkModel.slots[lod2Slot].lod2Fallback = false;
}

void ChunkManager::destructGrids() {
    ZoneScoped;
    KinematicBody& cameraBody = Registry.get<KinematicBody>(CameraController::activeCamera);

    glm::vec3 cameraPosition = cameraBody.position;
    glm::ivec2 initialGridCoords = GridHelper::worldToChunkIndex(cameraPosition);

    // Two hysteresis rings: collision demotes at 2 * lookUpRadius (chunk keeps rendering), the
    // full unload happens only past renderRadius + 2.
    int collisionRadius = 2 * TerrainConfig::lookUpRadius;
    int renderDestructRadius = TerrainConfig::renderRadius + 2;

    auto outsideWindow = [&](const ChunkSlot& slot, int radius) {
        return slot.aabbMin.x < (initialGridCoords.x - radius) * TerrainConfig::gridSize ||
               slot.aabbMin.z < (initialGridCoords.y - radius) * TerrainConfig::gridSize ||
               slot.aabbMax.x > (initialGridCoords.x + radius) * TerrainConfig::gridSize ||
               slot.aabbMax.z > (initialGridCoords.y + radius) * TerrainConfig::gridSize;
    };

    ZoneValue(chunkModel.maxLoadedChunks);
    int destructedCount = 0;
    bool didRun = false;
    for (size_t i = 0; i < chunkModel.maxLoadedChunks; i++) {
        auto& slot = chunkModel.slots[i];
        if (!slot.active) continue;

        glm::ivec2 gridCoords =
            GridHelper::worldToChunkIndex(glm::vec3(slot.aabbMin.x, 0, slot.aabbMin.z));

        if (outsideWindow(slot, renderDestructRadius)) {
            // If this chunk's triangles are still being built incrementally (spread across
            // multiple runIfNeeded() calls), cancel the in-flight task along with the rest of
            // the chunk's state - everything here runs synchronously on the main thread, so
            // there's no race to wait out, just state to clean up consistently.
            auto taskIt = genPipeline.triangleGenerationTasks.find(gridCoords);
            if (taskIt != genPipeline.triangleGenerationTasks.end() &&
                !taskIt->second.isGenerated) {
                genPipeline.triangleGenerationTasks.erase(taskIt);
            }

            didRun = true;
            destructedCount++;
            chunkModel.unloadChunk(static_cast<int>(i));
            chunkModel.heightMaps.erase(gridCoords);
            chunkCollisionGrid.erase(gridCoords);
            loadedChunks.erase(gridCoords);
            Triangles.remove(gridCoords);
        } else if (outsideWindow(slot, collisionRadius)) {
            // Demote: drop collision + LOD0/1 meshes, keep the LOD2 mesh and baked heightmap.
            if (slot.lod == 0 || slot.lod == 1) {
                chunkModel.unloadChunk(static_cast<int>(i));
            } else if (chunkCollisionGrid.find(gridCoords) != chunkCollisionGrid.end()) {
                auto taskIt = genPipeline.triangleGenerationTasks.find(gridCoords);
                if (taskIt != genPipeline.triangleGenerationTasks.end() &&
                    !taskIt->second.isGenerated) {
                    genPipeline.triangleGenerationTasks.erase(taskIt);
                }
                chunkCollisionGrid.erase(gridCoords);
                Triangles.remove(gridCoords);
                slot.lod2Fallback = true;
            }
        }
    }
    ZoneValue(destructedCount);

    if (didRun) {
        EnvironmentState::worldMin.x =
            (initialGridCoords.x - renderDestructRadius) * TerrainConfig::gridSize;
        EnvironmentState::worldMin.y =
            (initialGridCoords.y - renderDestructRadius) * TerrainConfig::gridSize;
        EnvironmentState::worldMax.x =
            (initialGridCoords.x + renderDestructRadius) * TerrainConfig::gridSize;
        EnvironmentState::worldMax.y =
            (initialGridCoords.y + renderDestructRadius) * TerrainConfig::gridSize;
    }
}

// distance < sqrt(pow(closestX, 2) + pow(closestZ, 2))
CollisionShapeGrid* ChunkManager::getTerrainAt(const glm::vec3& worldPosition) {
    auto gridCoords = GridHelper::worldToChunkIndex(worldPosition);
    auto it = chunkCollisionGrid.find(gridCoords);
    return it != chunkCollisionGrid.end() ? it->second.get() : nullptr;
}

bool ChunkManager::chunkExistsDeployed(const glm::ivec2& gridCoords) const {
    return chunkCollisionGrid.find(gridCoords) != chunkCollisionGrid.end();
}