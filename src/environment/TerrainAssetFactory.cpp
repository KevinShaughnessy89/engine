#include "environment/TerrainAssetFactory.hpp"

#include "PerlinNoise.hpp"
#include "components/rendering/InstanceLOD.hpp"
#include "components/rendering/InstancedRenderable.hpp"
#include "components/rendering/TerrainTreeImposter.hpp"
#include "core/Core.hpp"
#include "environment/ChunkManager.hpp"
#include "environment/CollisionShapeGrid.hpp"
#include "environment/EnvironmentState.hpp"
#include "environment/TreePlacementData.hpp"
#include "rendering/AssetFilePath.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/GLState.hpp"
#include "rendering/InstancedMeshVertex.hpp"
#include "rendering/InstancedModel.hpp"
#include "rendering/ModelLoader.hpp"
#include "rendering/Texture.hpp"
#include "rendering/TextureManager.hpp"
#include "rendering/TextureState.hpp"
#include "stb_image_write.h"

void ImposterBaker::init() {
    int atlasSize = gridRes * tileSize;
    atlasPixels.resize(atlasSize * atlasSize * 4, 0);

    bakeShader =
        new ShaderProgram(Config::instancedModelVertexPath, Config::imposterBakeFragmentPath);

    glGenFramebuffers(1, &captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, tileSize, tileSize, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    glGenRenderbuffers(1, &depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, tileSize, tileSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::vec3 ImposterBaker::octDecode(float u, float v) {
    glm::vec3 dir(u, v, 1.0f - std::abs(u) - std::abs(v));
    return glm::normalize(dir);
}

void ImposterBaker::blitTile(int col, int row, const std::vector<unsigned char>& pixels) {
    int atlasSize = gridRes * tileSize;
    for (int y = 0; y < tileSize; ++y) {
        int rowOffset = y * tileSize * 4;
        int destY = row * tileSize + y;
        int destRowOffset = (destY * atlasSize + col * tileSize) * 4;
        std::memcpy(&atlasPixels[destRowOffset], &pixels[rowOffset], tileSize * 4);
    }
}

void ImposterBaker::captureView(const glm::vec3& treeCenter, float boundingRadius,
                                const glm::vec3& viewDir, std::vector<unsigned char>& outPixels,
                                InstancedModel* model) {
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glViewport(0, 0, tileSize, tileSize);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // textureDiffuse is GL_SRGB8_ALPHA8, so sampling it always decodes to linear -- without this,
    // that linear value gets written straight into colorTexture (also GL_SRGB8_ALPHA8) with no
    // re-encode, so the bytes glReadPixels/stbi_write_png see are a full gamma curve too dark.
    glEnable(GL_FRAMEBUFFER_SRGB);

    // Reset every field to the GLStateFlags baseline rather than nudging off of GLState::current.
    GLState::setDepthFunc(GL_LESS);
    GLState::setDepthTest(true);
    GLState::setDepthMask(true);
    GLState::setCullFace(true);
    GLState::setDepthClamp(false);
    GLState::setBlend(true);
    GLState::setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLState::setWireframe(false);

    glm::vec3 eye = treeCenter + viewDir * boundingRadius * 2.0f;
    glm::vec3 up = std::abs(viewDir.y) > 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    glm::mat4 view = glm::lookAt(eye, treeCenter, up);

    glm::mat4 proj = glm::ortho(-boundingRadius, boundingRadius, -boundingRadius, boundingRadius,
                                0.01f, boundingRadius * 4.0f);

    bakeShader->use();
    bakeShader->setMat4("view", view);
    bakeShader->setMat4("projection", proj);

    for (auto& mesh : model->instancedMeshes) {
        mesh.render(bakeShader, TextureState::firstFreeTextureID);
    }

    outPixels.resize(static_cast<size_t>(tileSize * tileSize * 4));

    glReadPixels(0, 0, tileSize, tileSize, GL_RGBA, GL_UNSIGNED_BYTE, outPixels.data());

    glDisable(GL_FRAMEBUFFER_SRGB);
}

void ImposterBaker::saveAtlas(const std::string& path) {
    int atlasSize = gridRes * tileSize;
    // stbi expects rows top-to-bottom; glReadPixels returns bottom-to-top,
    // so flip vertically before writing or your baked tree will be upside down.
    std::vector<unsigned char> flipped(atlasPixels.size());
    int rowBytes = atlasSize * 4;
    for (int y = 0; y < atlasSize; ++y) {
        std::memcpy(&flipped[y * rowBytes], &atlasPixels[(atlasSize - 1 - y) * rowBytes], rowBytes);
    }
    stbi_write_png(path.c_str(), atlasSize, atlasSize, 4, flipped.data(), rowBytes);
}

void ImposterBaker::bakeTreeImposter(const glm::vec3 treeCenter, float boundingRadius,
                                     InstancedModel* model) {
    std::vector<unsigned char> pixels;

    for (int row = 0; row < gridRes; ++row) {
        for (int col = 0; col < gridRes; ++col) {
            float u = (static_cast<float>(col) + 0.5f) / static_cast<float>(gridRes);
            float v = (static_cast<float>(row) + 0.5f) / static_cast<float>(gridRes);
            glm::vec3 viewDir = octDecode(u * 2.0f - 1.0f, v * 2.0f - 1.0f);

            captureView(treeCenter, boundingRadius, viewDir, pixels, model);
            blitTile(col, row, pixels);
        }
    }
}

void ImposterBaker::cleanup() {
    glDeleteFramebuffers(1, &captureFBO);
    glDeleteTextures(1, &colorTexture);
    glDeleteRenderbuffers(1, &depthRBO);
    delete bakeShader;
    bakeShader = nullptr;
}

void TerrainAssetFactory::generateTreePlacements(
    std::unordered_map<glm::ivec2, std::unique_ptr<CollisionShapeGrid>, PairHash_1>&
        chunkCollisionGrid) {
    std::mt19937 rng(TerrainConfig::seed);
    std::uniform_real_distribution<float> unit(0.f, 1.f);

    float maxSlope = 0.2f;
    float groundFlushBias = 0.5f;  // Trees are shifted down to hide base geometry

    // Bark mesh bounding box includes branches, not just the trunk, so it's useless for
    // sizing a collision capsule - hand-tuned to roughly match trunk thickness instead.
    float pineTrunkRadius = 0.5f;
    float birchTrunkRadius = 0.5f;

    InstancedModel* pineModel = ModelRegistry.getInstancedModel("trees", 0);
    InstancedModel* birchModel = ModelRegistry.getInstancedModel("trees", 0);

    int treeCount = 0;

    for (int x = EnvironmentState::worldMin.x; x < EnvironmentState::worldMax.x; x += 10) {
        for (int z = EnvironmentState::worldMin.y; z < EnvironmentState::worldMax.y; z += 10) {
            float noise = PerlinNoise::perlin2D(x * 0.05f, z * 0.05f);
            if (noise < unit(rng)) {
                continue;
            }

            // Trees run in post-processing with every chunk loaded, so the baked map is present.
            float height;
            if (!Chunks.sampleBakedHeight(x, z, height)) continue;
            glm::vec3 normal = Chunks.sampleBakedNormal(x, z);
            if (glm::dot(normal, glm::vec3(0.f, 1.f, 0.f)) < maxSlope) {
                continue;
            }

            TreeType type = (unit(rng) < 0.7f) ? TreeType::Pine : TreeType::Birch;
            InstancedModel* model = (type == TreeType::Pine) ? pineModel : birchModel;
            InstancedModel* imposterModel =
                (type == TreeType::Pine) ? pineImposterModel : birchImposterModel;
            if (!model) continue;

            // Tree trunk collision disabled: calculateGeometricDataForMaterial("bark") doesn't
            // match any material in the current trees.fbx (materials are bush2, bushflower,
            // bushType1, pine2, pine3, Trunk, tree2, tree3), so this was building every
            // capsule from infinite/degenerate bounds. Re-enable once the correct trunk
            // material name(s) are wired in.
            // auto e = Registry.create();
            // ShapeData& shapeData = Registry.emplace<ShapeData>(e);
            // auto& capsuleData = ColliderFactory::addCapsule(Registry, e);
            // Capsule::defineBoundingVolume(
            //     capsuleData, type == TreeType::Pine
            //                      ? pineModel->calculateGeometricDataForMaterial("bark")
            //                      : birchModel->calculateGeometricDataForMaterial("bark"));
            // capsuleData.radius = type == TreeType::Pine ? pineTrunkRadius : birchTrunkRadius;
            // capsuleData.currentCenter += glm::vec3(x, height, z);
            // shapeData.isStatic = true;

            // glm::ivec2 gridCoords = glm::ivec2(
            //     static_cast<int>(std::floor(static_cast<float>(x) /
            //     TerrainConfig::gridSize)), static_cast<int>(std::floor(static_cast<float>(z)
            //     / TerrainConfig::gridSize)));

            // if (chunkCollisionGrid.find(gridCoords) == chunkCollisionGrid.end()) {
            //     chunkCollisionGrid[gridCoords] = std::make_unique<CollisionShapeGrid>(
            //         glm::vec2(gridCoords.x * TerrainConfig::gridSize,
            //                   gridCoords.y * TerrainConfig::gridSize),
            //         glm::vec2((gridCoords.x + 1) * TerrainConfig::gridSize,
            //                   (gridCoords.y + 1) * TerrainConfig::gridSize),
            //         std::vector<uint32_t>());
            // }

            // chunkCollisionGrid[gridCoords]->insertObject(e);

            // Trees are shifted down to patch a bug where they don't sit on the terrain surface
            // correctly
            InstancedMeshVertex placement{glm::vec3(x, height, z), glm::vec3(1.0f),
                                          glm::vec3(1.0f)};

            glm::vec3 max = model->getMaxCoordinates();
            glm::vec3 min = model->getMinCoordinates();

            placement.position.y -= (min.y + groundFlushBias);

            model->allInstances.push_back(placement);

            if (imposterModel) {
                // Offset by the tree model's true center -- instancePosition is the tree's
                // base, but the imposter quad/atlas is built around that center, not the base.
                const glm::vec3& centerOffset =
                    imposterCenterOffset[type == TreeType::Pine ? 0 : 1];
                InstancedMeshVertex imposterPlacement = placement;
                imposterPlacement.position += centerOffset;
                imposterPlacement.position.y =
                    placement.position.y + centerOffset.y - min.y - groundFlushBias;
                imposterModel->allInstances.push_back(imposterPlacement);
            }

            treeCount++;
        }
    }

    std::cout << "Final trees size " << treeCount << std::endl;
}

void TerrainAssetFactory::bakeTreeImposters() {
    pineImposterModel = createTreeImposter("trees", 0, "tree_imposter_0");
    birchImposterModel = createTreeImposter("trees", 0, "tree_imposter_1");
}

InstancedModel* TerrainAssetFactory::createTreeImposter(const std::string& modelName, int index,
                                                        const std::string& imposterName) {
    std::string path =
        Config::ProjectRootDir + "/" + AssetFilePath::terrainTextures + imposterName + ".png";

    InstancedModel* model = ModelRegistry.getInstancedModel(modelName, index);

    glm::vec3 min = model->getMinCoordinates();
    glm::vec3 max = model->getMaxCoordinates();
    glm::vec3 extent = max - min;
    float boundingRadius = std::max({extent.x, extent.y, extent.z}) * 0.5f;
    glm::vec3 center = (min + max) * 0.5f;

    if (!std::filesystem::exists(path)) {
        model->updateVisibleInstances({InstancedMeshVertex{glm::vec3(0.0f), glm::vec3(1.0f)}});

        baker.init();
        baker.bakeTreeImposter(center, boundingRadius, model);
        baker.saveAtlas(path);
        baker.cleanup();
    }

    Texture* atlasTexture;
    if (Texture* existing = Textures.getTexture(imposterName)) {
        atlasTexture = existing;
    } else {
        std::string filename = imposterName + ".png";
        TextureData atlasData(path, imposterName, filename);
        atlasTexture = new Texture2D(std::vector<TextureData*>{&atlasData}, imposterName,
                                     "treeAtlasTexture");
    }

    atlasTexture->setUniform("treeAtlasTexture");
    // Quad stays symmetric around its own origin -- the offset from instancePosition (the
    // tree's base) to the model's true center (the point captureView() orbits around when
    // baking) is instead baked into imposterCenterOffset below and applied to instancePosition
    // itself in generateTreePlacements(), so it holds for X/Z as well as Y.
    std::vector<MeshVertex> quadVertices = {
        {{-boundingRadius, boundingRadius, 0.0f, 1.0f}, {0, 0, 0}, {0, 1}, {0, 0, 0}, {0, 0, 0}},
        {{-boundingRadius, -boundingRadius, 0.0f, 1.0f}, {0, 0, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{boundingRadius, -boundingRadius, 0.0f, 1.0f}, {0, 0, 0}, {1, 0}, {0, 0, 0}, {0, 0, 0}},
        {{boundingRadius, boundingRadius, 0.0f, 1.0f}, {0, 0, 0}, {1, 1}, {0, 0, 0}, {0, 0, 0}},
    };
    std::vector<unsigned int> quadIndices = {0, 1, 2, 0, 2, 3};

    imposterCenterOffset[index] = center;

    imposterModels.push_back(std::make_unique<InstancedModel>(
        std::move(quadVertices), std::move(quadIndices), atlasTexture));
    InstancedModel* imposterModel = imposterModels.back().get();

    // getMinCoordinates()/getMaxCoordinates() are normally filled in per-vertex by
    // ModelLoader::processMesh() while parsing an assimp scene -- this quad is built directly,
    // so cullInstances() would otherwise see the +-infinity defaults and never cull it.
    imposterModel->min[0] = -boundingRadius;
    imposterModel->max[0] = boundingRadius;
    imposterModel->min[1] = -boundingRadius;
    imposterModel->max[1] = boundingRadius;
    imposterModel->min[2] = 0.0f;
    imposterModel->max[2] = 0.0f;

    entt::entity treeEntity = ModelRegistry.getEntity(modelName, index);

    if (Registry.all_of<InstanceLOD>(treeEntity) &&
        Registry.all_of<InstancedRenderable>(treeEntity)) {
        std::cerr
            << "Warning: Tree entity already has InstanceLOD and InstancedRenderable components. "
               "Skipping adding new ones."
            << std::endl;
    } else {
        Registry.emplace<InstanceLOD>(treeEntity, TerrainConfig::treeImposterDistanceLOD, false);

        entt::entity imposterEntity = Registry.create();
        Registry.emplace<InstancedRenderable>(imposterEntity, imposterModel,
                                              Display.getImposterModelShaderID(),
                                              RenderLayer::TreeImposters, true);
        Registry.emplace<TerrainTreeImposter>(imposterEntity, baker.gridRes, index);
        Registry.emplace<InstanceLOD>(imposterEntity, TerrainConfig::treeImposterDistanceLOD, true);
    }

    return imposterModel;
}
