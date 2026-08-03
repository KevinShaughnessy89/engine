#pragma once

#include <string>

#include "Material.hpp"
#include "ModelType.hpp"
#include "character/AnimationRenderer.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "rendering/MeshStructs.hpp"
#include "rendering/RenderLayer.hpp"
#include "rendering/ShaderType.hpp"

class Model;
struct Renderable;
struct InstancedRenderable;
class InstancedModel;
class Texture;

struct ModelData {
    entt::entity entity;
    std::string name;
    GLuint shaderID;
    std::string path;
    ShaderType shaderType;
    RenderLayer renderLayer;
    ModelType modelType;
    bool mergeSiblings = false;
    float forwardOffsetDegrees = 0.0f;
    std::vector<std::pair<std::string, std::string>> uniforms;
    std::vector<std::pair<std::string, std::string>> animations;
};

class ModelLoader {
   public:
    ModelLoader();
    ~ModelLoader();

    void create(const std::string& name, ModelData& data);
    void loadScene(const std::string& path);
    Model* getModel(const std::string& name, int index = 0);
    InstancedModel* getInstancedModel(const std::string& name, int index = 0);
    void createModel(ModelData& data, std::unique_ptr<Model> modelPtr, const aiScene* scene,
                     aiNode* node);
    void createInstancedModel(ModelData& data, std::unique_ptr<InstancedModel> modelPtr,
                              const aiScene* scene, aiNode* node);
    void processAnimations(const ModelData& data);
    Renderable& getRenderable(const std::string& name, int index = 0);
    InstancedRenderable& getInstancedRenderable(const std::string& name, int index = 0);
    entt::entity getEntity(const std::string& name, int index = 0);
    void PrintScene(const aiScene* scene, bool detailed = true);
    void PrintNode(const aiScene* scene, const aiNode* node, int depth = 0, bool detailed = true);
    void PrintMeshSummary(const aiScene* scene, bool detailed = true);
    void PrintMaterialSummary(const aiScene* scene);
    void PrintSceneMetadata(const aiScene* scene);
    void PrintMetadataEntry(const aiMetadata* metadata, unsigned i);
    void PrintTextureTypes(const aiMaterial* material);

    std::vector<std::unique_ptr<Model>> models;
    std::vector<std::unique_ptr<InstancedModel>> instancedModels;
    std::unordered_map<std::string, std::vector<entt::entity>> entities;

   private:
    Assimp::Importer importer;
    std::unordered_map<unsigned int, size_t> materialCache;
    std::unordered_map<unsigned int, Texture*> embeddedTextures;

    std::vector<std::pair<std::string, std::string>> parseUniforms(const rapidjson::Value& value);

    void finalizeModelEntity(ModelData& data, std::unique_ptr<Model> modelPtr,
                             const aiScene* scene);
    void finalizeModelEntity(ModelData& data, std::unique_ptr<InstancedModel> modelPtr,
                             const aiScene* scene);

    void loadModel(Model* model);
    template <typename ModelT>
    void processNode(ModelT* model, aiNode* node, const aiScene* scene);
    template <typename ModelT>
    void processMesh(ModelT* model, aiMesh* mesh, const aiScene* scene, const glm::mat4& transform,
                     const glm::mat3& normalMatrix);
    void createMesh(Model* model, std::vector<MeshVertex>& vertices,
                    std::vector<unsigned int>& indices, std::vector<Texture*>& textures,
                    std::string material, size_t materialIndex);
    void createMesh(InstancedModel* model, std::vector<MeshVertex>& vertices,
                    std::vector<unsigned int>& indices, std::vector<Texture*>& textures,
                    std::string material, size_t materialIndex);
    std::vector<Texture*> loadMaterialTextures(aiMaterial* mat, const aiScene* scene,
                                               aiTextureType type);
    Material extractMaterial(aiMaterial* material);
    void loadEmbeddedTextures(const aiScene* scene);
    Texture* createTextureFromEmbedded(const aiTexture* tex, unsigned int index);
    template <typename ModelT>
    size_t getOrCreateMaterialIndex(ModelT* model, aiMaterial* material,
                                    unsigned int sceneMaterialIndex);
};
