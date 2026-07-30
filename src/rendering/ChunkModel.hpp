#pragma once

#include "common/PairHash.hpp"
#include "core/Clipmap.hpp"
#include "core/PrecompiledHeader.hpp"

class Texture;
class TextureArray2D;
class TextureDataArray2D;
class ShaderProgram;

struct ChunkSlot {
    bool active = false;
    bool shouldRender = false;
    int lod = 0;
    // Render-only chunks carry just a LOD2 mesh; this flag makes it draw at any LOD distance
    // until the chunk is promoted with full LOD0/1 meshes (ChunkManager::promoteChunkCollision).
    bool lod2Fallback = false;
    glm::vec3 aabbMin, aabbMax;
    GLint baseVertex;
    GLuint indexCount;
};

class ChunkModel {
   public:
    ChunkModel();
    // Safe to call again after the first time (e.g. when TerrainConfig::triangleCount changes at
    // runtime) -- frees the previous GL buffers before reallocating at the current sizes.
    // collisionChunks sizes the LOD0/1 pools; renderOnlyChunks adds LOD2-only capacity on top.
    void initialize(int collisionChunks, int renderOnlyChunks);
    int loadChunk(const std::vector<glm::vec3>& vertices, const glm::vec3& aabbMin,
                  const glm::vec3& aabbMax, int lod, bool lod2Fallback = false);
    void unloadChunk(int slot);
    // Finds the active slot for a chunk by its aabb XZ origin and LOD tier; -1 when absent.
    int findSlot(const glm::vec3& aabbMin, int lod) const;
    std::vector<GLuint> generateIndicesForLod(int lod);
    void setLod(ChunkSlot& slot, int lod);
    // issues one draw call per visible chunk; ignoreCulling draws every active slot regardless of
    // shouldRender, for passes (e.g. the shadow map) whose visibility isn't the main camera's.
    // bindTextures should be false for depth-only passes, whose shader has no sampler uniforms to
    // bind against.
    void render(ShaderProgram* shader, GLuint firstFreeTextureId, bool ignoreCulling = false,
                bool bindTextures = true);
    std::vector<ChunkSlot> slots;  // indexed by slot number
    std::vector<Texture*> textures;
    // Baked per-chunk heightmaps keyed by gridCoord: flat (triangleCount+3)^2 grid, GL row-major
    // (index = z * width + x). Sampled via HeightMapGenerator::bilerpHeightAt.
    std::unordered_map<glm::ivec2, std::vector<float>, PairHash_1> heightMaps;
    size_t maxLoadedChunks;

    // Global per-frame resource (not per-entity material data): bound directly at a fixed reserved
    // texture unit in render() -- via the normal Texture::bindTexture() path, just called
    // unconditionally rather than gated behind render()'s bindTextures flag -- because the gbuffer
    // pass calls render(..., bindTextures=false) (it doesn't want chunkModel's other material
    // samplers bound) but still needs normalMapBuffer for its own displacement/normal calculation.
    // Precomputed per-texel normal (XZ only, Y reconstructed in-shader): baked on a worker thread
    // in TerrainMeshGenerator::calculateNormals and uploaded per-chunk (see
    // ChunkGenerationPipeline::createChunk). Tangent/bitangent are derived in-shader from this
    // sampled normal rather than stored, since they're pure functions of it.
    Texture* normalMapBufferTexture;
    TextureDataArray2D* terrainNormalMapArray;
    TextureArray2D* terrainDiffuseMapArray;
    Clipmap<glm::vec2> normalMapBuffer;

   private:
    GLuint sharedVAO_lod0, sharedVBO_lod0, sharedEBO_lod0;
    GLuint sharedVAO_lod1, sharedVBO_lod1, sharedEBO_lod1;
    GLuint sharedVAO_lod2, sharedVBO_lod2, sharedEBO_lod2;

    int normalBufferTextureUnit = 24;

    GLuint indirectBuffer;

    size_t verticesPerChunk_lod0, verticesPerChunk_lod1, verticesPerChunk_lod2;
    size_t sharedIndexCount_lod0, sharedIndexCount_lod1, sharedIndexCount_lod2;

    std::vector<int> freeSlots;
    // Per-LOD VBO region pools: a slot's baseVertex comes from its LOD's pool, so LOD2-only
    // render chunks don't reserve LOD0-sized buffer space.
    size_t regionCount_lod0, regionCount_lod1, regionCount_lod2;
    std::vector<int> freeRegions_lod0, freeRegions_lod1, freeRegions_lod2;

    bool initialized = false;
    void teardown();
    void buildSharedIndices();
};