#pragma once

#include "common/PairHash.hpp"
#include "components/collision/ShapeData.hpp"

// Owns TriangleData for every terrain triangle in the world. Triangles are grouped into
// per-chunk slots - a fixed-capacity vector of vectors, resized once up front via initialize()
// and never reallocated after that, mirroring ChunkModel's slots/freeSlots pattern - so a
// chunk's triangles can be dropped in one shot when the chunk is destructed while keeping the
// hot get()-by-id path (called from the FixedRateClock collision thread) a plain, lock-free
// vector index.
// A triangle id packs its chunk slot into the high bits and its index within that chunk's
// triangle list into the low bits. allocateChunk()/create()/remove() are only ever called from
// the main thread (chunk streaming, Game::updateWorld), same as entt::registry::create() is
// today.
class TriangleManager {
   public:
    TriangleManager() = default;

    void initialize(size_t maxLoadedChunks);

    size_t allocateChunk(const glm::ivec2& gridCoords);
    uint32_t create(size_t chunkSlot, TriangleData data);

    TriangleData& get(uint32_t triangleId);
    const TriangleData& get(uint32_t triangleId) const;

    void remove(const glm::ivec2& gridCoords);

   private:
    static constexpr uint32_t indexBits = 16;
    static constexpr uint32_t indexMask = (1u << indexBits) - 1;

    std::vector<std::vector<TriangleData>> chunkTriangles;
    std::vector<size_t> freeSlots;
    std::unordered_map<glm::ivec2, size_t, PairHash_1> gridToSlot;
};
