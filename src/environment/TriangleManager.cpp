#include "TriangleManager.hpp"

void TriangleManager::initialize(size_t maxLoadedChunks) {
    // gridToSlot isn't touched by resize() below -- clear it explicitly so a reinitialize() call
    // (terrain regenerated at a new triangleCount) doesn't leave stale grid-coord -> slot entries
    // pointing at slots that are about to be handed out again from scratch.
    gridToSlot.clear();
    chunkTriangles.resize(maxLoadedChunks);
    freeSlots.resize(maxLoadedChunks);
    for (size_t i = 0; i < maxLoadedChunks; i++) freeSlots[i] = i;
}

size_t TriangleManager::allocateChunk(const glm::ivec2& gridCoords) {
    size_t slot = freeSlots.back();
    freeSlots.pop_back();
    chunkTriangles[slot].clear();
    gridToSlot[gridCoords] = slot;
    return slot;
}

uint32_t TriangleManager::create(size_t chunkSlot, TriangleData data) {
    auto& triangles = chunkTriangles[chunkSlot];
    uint32_t localIndex = static_cast<uint32_t>(triangles.size());
    data.triangleIndex = localIndex;
    triangles.push_back(std::move(data));
    return (static_cast<uint32_t>(chunkSlot) << indexBits) | localIndex;
}

TriangleData& TriangleManager::get(uint32_t triangleId) {
    return chunkTriangles[triangleId >> indexBits][triangleId & indexMask];
}

const TriangleData& TriangleManager::get(uint32_t triangleId) const {
    return chunkTriangles[triangleId >> indexBits][triangleId & indexMask];
}

void TriangleManager::remove(const glm::ivec2& gridCoords) {
    auto it = gridToSlot.find(gridCoords);
    if (it == gridToSlot.end()) return;

    chunkTriangles[it->second].clear();
    freeSlots.push_back(it->second);
    gridToSlot.erase(it);
}
