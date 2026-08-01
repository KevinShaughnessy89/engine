#include "ChunkModel.hpp"

#include <glm/ext/vector_int2.hpp>
#include <iostream>
#include <tracy/Tracy.hpp>

#include "TextureManager.hpp"
#include "components/rendering/ViewState.hpp"
#include "core/Config.hpp"
#include "core/Core.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "environment/TerrainConfig.hpp"
#include "environment/TerrainMeshGenerator.hpp"
#include "rendering/AssetFilePath.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/ShaderProgram.hpp"
#include "rendering/Texture.hpp"

namespace {
// Each chunk keeps all 3 LOD tiers permanently resident as separate slots (see
// ChunkModel::initialize's maxLoadedChunks comment); the correct tier to draw is picked here by
// distance rather than by mutating a slot's .lod, since a slot's .lod/baseVertex pairing is fixed
// to whichever shared buffer its vertex data was actually uploaded into at load time.
int desiredLodForDistance(float distance) {
    if (distance > TerrainConfig::lod2Distance) return 2;
    if (distance > TerrainConfig::lod1Distance) return 1;
    return 0;
}
}  // namespace

ChunkModel::ChunkModel() : initialized(false) {
}

void ChunkModel::teardown() {
    glDeleteBuffers(1, &sharedVBO_lod0);
    glDeleteBuffers(1, &sharedVBO_lod1);
    glDeleteBuffers(1, &sharedVBO_lod2);
    glDeleteBuffers(1, &sharedEBO_lod0);
    glDeleteBuffers(1, &sharedEBO_lod1);
    glDeleteBuffers(1, &sharedEBO_lod2);
    glDeleteVertexArrays(1, &sharedVAO_lod0);
    glDeleteVertexArrays(1, &sharedVAO_lod1);
    glDeleteVertexArrays(1, &sharedVAO_lod2);
    delete normalMapBufferTexture;
}

void ChunkModel::initialize(int collisionChunks, int renderOnlyChunks) {
    if (initialized) teardown();
    initialized = true;

    // Sized to the render-destruct hysteresis radius (renderRadius+2, see destructGrids), not
    // renderRadius itself -- same worst-case-window reasoning as regionCount_lod2/maxLoadedChunks,
    // so two simultaneously-loaded chunks can never wrap to overlapping texel regions.
    normalMapBuffer.init(
        2 * (TerrainConfig::renderRadius + 2) * (TerrainConfig::triangleCount + 3),
        static_cast<float>(TerrainConfig::gridSize) / TerrainConfig::triangleCount);

    verticesPerChunk_lod0 = (TerrainConfig::triangleCount + 1) * (TerrainConfig::triangleCount + 1);
    verticesPerChunk_lod1 =
        ((TerrainConfig::triangleCount + 1) * (TerrainConfig::triangleCount + 1) / 2);
    verticesPerChunk_lod2 =
        ((TerrainConfig::triangleCount + 1) * (TerrainConfig::triangleCount + 1) / 4);

    // Only collision-window chunks carry LOD0/1 meshes; LOD2 covers the whole render window.
    regionCount_lod0 = collisionChunks;
    regionCount_lod1 = collisionChunks;
    regionCount_lod2 = collisionChunks + renderOnlyChunks;
    maxLoadedChunks = regionCount_lod0 + regionCount_lod1 + regionCount_lod2;

    slots.assign(maxLoadedChunks, ChunkSlot{});
    freeSlots.resize(maxLoadedChunks);
    for (int i = 0; i < maxLoadedChunks; i++) freeSlots[i] = i;

    freeRegions_lod0.resize(regionCount_lod0);
    for (size_t i = 0; i < regionCount_lod0; i++) freeRegions_lod0[i] = static_cast<int>(i);
    freeRegions_lod1.resize(regionCount_lod1);
    for (size_t i = 0; i < regionCount_lod1; i++) freeRegions_lod1[i] = static_cast<int>(i);
    freeRegions_lod2.resize(regionCount_lod2);
    for (size_t i = 0; i < regionCount_lod2; i++) freeRegions_lod2[i] = static_cast<int>(i);

    glGenBuffers(1, &sharedVBO_lod0);
    glBindBuffer(GL_ARRAY_BUFFER, sharedVBO_lod0);
    glBufferData(GL_ARRAY_BUFFER, regionCount_lod0 * verticesPerChunk_lod0 * sizeof(glm::vec3),
                 nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &sharedVBO_lod1);
    glBindBuffer(GL_ARRAY_BUFFER, sharedVBO_lod1);
    glBufferData(GL_ARRAY_BUFFER, regionCount_lod1 * verticesPerChunk_lod1 * sizeof(glm::vec3),
                 nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &sharedVBO_lod2);
    glBindBuffer(GL_ARRAY_BUFFER, sharedVBO_lod2);
    glBufferData(GL_ARRAY_BUFFER, regionCount_lod2 * verticesPerChunk_lod2 * sizeof(glm::vec3),
                 nullptr, GL_DYNAMIC_DRAW);

    // Every chunk shares the same grid topology, so the index buffer is built once here
    // rather than per-chunk; loadChunk() only ever uploads vertex positions.
    glGenBuffers(1, &sharedEBO_lod0);
    glGenBuffers(1, &sharedEBO_lod1);
    glGenBuffers(1, &sharedEBO_lod2);

    glGenVertexArrays(1, &sharedVAO_lod0);
    glBindVertexArray(sharedVAO_lod0);
    glBindBuffer(GL_ARRAY_BUFFER, sharedVBO_lod0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO_lod0);

    glGenVertexArrays(1, &sharedVAO_lod1);
    glBindVertexArray(sharedVAO_lod1);
    glBindBuffer(GL_ARRAY_BUFFER, sharedVBO_lod1);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO_lod1);

    glGenVertexArrays(1, &sharedVAO_lod2);
    glBindVertexArray(sharedVAO_lod2);
    glBindBuffer(GL_ARRAY_BUFFER, sharedVBO_lod2);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO_lod2);

    glPatchParameteri(GL_PATCH_VERTICES,
                      3);  // Set here since ChunkModel is the only user of tessellation shaders in
                           // the engine, and it always uses 3 control points per patch.

    buildSharedIndices();

    terrainDiffuseMapArray = new TextureArray2D("terrain_diffuse", 4, "textureDiffuseArray");

    terrainNormalMapArray = new TextureDataArray2D("terrain_normal", 4, "textureNormalArray");

    std::string rootPath = Config::ProjectRootDir + "/" + AssetFilePath::terrainTextures;
    // terrainDiffuseMapArray->loadTexture(rootPath + "/terrain_diffuse.png");
    terrainDiffuseMapArray->loadTexture(rootPath + "/forest_diffuse.png");
    terrainDiffuseMapArray->loadTexture(rootPath + "/rock_wall_diffuse.png");
    terrainDiffuseMapArray->loadTexture(rootPath + "/mossy_diffuse.png");

    // terrainNormalMapArray->loadTexture(rootPath + "/data_terrain_normal.png");
    terrainNormalMapArray->loadTexture(rootPath + "/data_forest_normal.png");
    terrainNormalMapArray->loadTexture(rootPath + "/data_rock_wall_normal.png");
    terrainNormalMapArray->loadTexture(rootPath + "/data_mossy_normal.png");

    // Todo: put all other map types into a texture array
    Texture* displacementMap = Textures.getTexture("data_terrain_displacement");
    displacementMap->setUniform("textureDisplacement");

    normalMapBufferTexture =
        new Texture2D(normalMapBuffer.getTextureID(), "normalMapBuffer", "normalMapBuffer");

    textures = {terrainDiffuseMapArray, terrainNormalMapArray, displacementMap};
}

void ChunkModel::setLod(ChunkSlot& slot, int lod) {
    slot.lod = lod;
    if (lod == 0) {
        slot.indexCount = sharedIndexCount_lod0;  // Use the index count for LOD 0
    } else if (lod == 1) {
        slot.indexCount = sharedIndexCount_lod1;  // Use the index count for LOD 1
    } else if (lod == 2) {
        slot.indexCount = sharedIndexCount_lod2;  // Use the index count for LOD 2
    }
}

std::vector<GLuint> ChunkModel::generateIndicesForLod(int indexCount) {
    std::vector<GLuint> indices;
    int rowWidth = indexCount + 1;
    indices.reserve(indexCount * indexCount * 6);

    for (int z = 0; z < indexCount; z++) {
        for (int x = 0; x < indexCount; x++) {
            GLuint topLeft = z * rowWidth + x;
            GLuint topRight = topLeft + 1;
            GLuint bottomLeft = (z + 1) * rowWidth + x;
            GLuint bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    return indices;
}

void ChunkModel::buildSharedIndices() {
    std::vector<GLuint> indices_lod0 = generateIndicesForLod(TerrainConfig::triangleCount);
    sharedIndexCount_lod0 = indices_lod0.size();
    std::vector<GLuint> indices_lod1 = generateIndicesForLod(TerrainConfig::triangleCount / 2);
    sharedIndexCount_lod1 = indices_lod1.size();
    std::vector<GLuint> indices_lod2 = generateIndicesForLod(TerrainConfig::triangleCount / 4);
    sharedIndexCount_lod2 = indices_lod2.size();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO_lod0);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_lod0.size() * sizeof(GLuint), indices_lod0.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO_lod1);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_lod1.size() * sizeof(GLuint), indices_lod1.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO_lod2);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_lod2.size() * sizeof(GLuint), indices_lod2.data(),
                 GL_STATIC_DRAW);
}

int ChunkModel::loadChunk(const std::vector<glm::vec3>& vertices, const glm::vec3& aabbMin,
                          const glm::vec3& aabbMax, int lod, bool lod2Fallback) {
    ZoneScoped;
    ZoneValue(vertices.size());
    int slot = freeSlots.back();
    freeSlots.pop_back();

    std::vector<int>& regions = lod == 0   ? freeRegions_lod0
                                : lod == 1 ? freeRegions_lod1
                                           : freeRegions_lod2;
    size_t verticesPerChunk = lod == 0   ? verticesPerChunk_lod0
                              : lod == 1 ? verticesPerChunk_lod1
                                         : verticesPerChunk_lod2;
    int region = regions.back();
    regions.pop_back();

    slots[slot].active = true;
    slots[slot].shouldRender = false;
    slots[slot].aabbMin = aabbMin;
    slots[slot].aabbMax = aabbMax;
    slots[slot].lod = lod;
    slots[slot].lod2Fallback = lod2Fallback;
    slots[slot].baseVertex = static_cast<GLint>(region * verticesPerChunk);
    slots[slot].indexCount = (lod == 0)   ? sharedIndexCount_lod0
                             : (lod == 1) ? sharedIndexCount_lod1
                                          : sharedIndexCount_lod2;

    if (lod == 0) {
        glBindBuffer(GL_ARRAY_BUFFER, sharedVBO_lod0);
    } else if (lod == 1) {
        glBindBuffer(GL_ARRAY_BUFFER, sharedVBO_lod1);
    } else if (lod == 2) {
        glBindBuffer(GL_ARRAY_BUFFER, sharedVBO_lod2);
    }

    glBufferSubData(GL_ARRAY_BUFFER, region * verticesPerChunk * sizeof(glm::vec3),
                    vertices.size() * sizeof(glm::vec3), vertices.data());

    return slot;
}

void ChunkModel::unloadChunk(int slot) {
    ZoneScoped;
    ChunkSlot& s = slots[slot];
    size_t verticesPerChunk = s.lod == 0   ? verticesPerChunk_lod0
                              : s.lod == 1 ? verticesPerChunk_lod1
                                           : verticesPerChunk_lod2;
    std::vector<int>& regions = s.lod == 0   ? freeRegions_lod0
                                : s.lod == 1 ? freeRegions_lod1
                                             : freeRegions_lod2;
    regions.push_back(static_cast<int>(s.baseVertex / verticesPerChunk));

    s.active = false;
    s.lod2Fallback = false;
    freeSlots.push_back(slot);
}

int ChunkModel::findSlot(const glm::vec3& aabbMin, int lod) const {
    for (size_t i = 0; i < maxLoadedChunks; i++) {
        const ChunkSlot& s = slots[i];
        if (s.active && s.lod == lod && s.aabbMin.x == aabbMin.x && s.aabbMin.z == aabbMin.z) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void ChunkModel::render(ShaderProgram* shader, GLuint firstFreeTextureId, bool ignoreCulling,
                        bool bindTextures) {
    ZoneScoped;
    bool anyVisible = false;
    for (auto& s : slots) {
        if (s.active && ((ignoreCulling || s.shouldRender))) {
            anyVisible = true;
            break;
        }
    }
    if (!anyVisible) return;

    if (bindTextures) {
        for (unsigned int i = 0; i < textures.size(); i++) {
            textures[i]->bindTexture(shader, firstFreeTextureId + i);
        }
    }

    ViewState& view = Registry.get<ViewState>(CameraController::activeCamera);
    glm::vec3 cameraPosition = view.position;

    shader->setFloat("chunkWorldSize", static_cast<float>(TerrainConfig::gridSize));
    shader->setFloat("worldTexelSize", normalMapBuffer.texelSize);
    shader->setFloat("normalMapBufferSize", static_cast<float>(normalMapBuffer.bufferSize));
    // Unconditional (not gated behind bindTextures) -- the gbuffer/SSAO pass calls render() with
    // bindTextures=false to skip chunkModel's other material samplers, but still needs this one.
    normalMapBufferTexture->bindTexture(shader, normalBufferTextureUnit);

    {
        ZoneScopedN("ChunkModel::render: LOD draw submission");
        ZoneValue(slots.size());
        for (int Lod = 0; Lod <= 2; Lod++) {
            if (Lod == 0) {
                glBindVertexArray(sharedVAO_lod0);
            } else if (Lod == 1) {
                glBindVertexArray(sharedVAO_lod1);
            } else if (Lod == 2) {
                glBindVertexArray(sharedVAO_lod2);
            }

            for (auto& s : slots) {
                if (!s.active || (!ignoreCulling && !s.shouldRender) || s.lod != Lod) continue;

                // XZ distance to the chunk's nearest AABB point: corner-based 3D distance made the
                // tier rings anisotropic (diagonally scattered) and shift with camera altitude.
                glm::vec2 camXZ(cameraPosition.x, cameraPosition.z);
                glm::vec2 nearest = glm::clamp(camXZ, glm::vec2(s.aabbMin.x, s.aabbMin.z),
                                               glm::vec2(s.aabbMax.x, s.aabbMax.z));
                float distance = glm::distance(camXZ, nearest);
                // lod2Fallback slots are a render-only chunk's sole mesh -- draw at any distance.
                if (desiredLodForDistance(distance) != Lod && !s.lod2Fallback) continue;

                glDrawElementsBaseVertex(GL_PATCHES, s.indexCount, GL_UNSIGNED_INT, nullptr,
                                         s.baseVertex);
            }
        }
    }
}