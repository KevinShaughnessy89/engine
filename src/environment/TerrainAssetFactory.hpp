#pragma once

#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/PairHash.hpp"
#include "core/Config.hpp"
#include "environment/TerrainConfig.hpp"
#include "rendering/ShaderProgram.hpp"

class InstancedModel;
class CollisionShapeGrid;

struct ImposterBaker {
    GLuint captureFBO = 0;
    GLuint depthRBO = 0;
    int tileSize = 128;
    GLuint colorTexture = 0;

    int gridRes = 8;                         // e.g. 8 -> 8x8 = 64 views
    std::vector<unsigned char> atlasPixels;  // gridRes*tileSize square, RGBA

    // Baking runs from EnvironmentManager::init(), before SunState is populated and before
    // Display.init() generates the IBL irradiance map, so the full PBR instancedModel shader
    // (which reads sunColor/ambientStrength/irradianceMap/lightPos) isn't usable here -- those
    // uniforms would all still be at their GL-default zero. This shader just unlit-samples
    // textureDiffuse with the same alpha cutout the real shader uses, so the baked atlas is the
    // plain albedo rather than solid black.
    ShaderProgram* bakeShader = nullptr;

    void init();
    glm::vec3 octDecode(float u, float v);
    void blitTile(int col, int row, const std::vector<unsigned char>& pixels);
    void captureView(const glm::vec3& treeCenter, float boundingRadius, const glm::vec3& viewDir,
                     std::vector<unsigned char>& outPixels, InstancedModel* model);
    void saveAtlas(const std::string& path);
    void bakeTreeImposter(const glm::vec3 treeCenter, float boundingRadius, InstancedModel* model);
    void cleanup();
};

class TerrainAssetFactory {
   public:
    ImposterBaker baker;
    InstancedModel* pineImposterModel = nullptr;
    InstancedModel* birchImposterModel = nullptr;
    glm::vec3 imposterCenterOffset[2] = {glm::vec3(0.0f), glm::vec3(0.0f)};

    void generateTreePlacements(std::unordered_map<glm::ivec2, std::unique_ptr<CollisionShapeGrid>,
                                                   PairHash_1>& chunkCollisionGrid);
    void bakeTreeImposters();

   private:
    std::vector<std::unique_ptr<InstancedModel>> imposterModels;

    InstancedModel* createTreeImposter(const std::string& modelName, int index,
                                       const std::string& imposterName);
};
