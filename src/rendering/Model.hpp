#pragma once
#include "Material.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "core/PrecompiledHeader.hpp"

class TerrainTriangle;
struct GeometricData;
class ShaderProgram;

class Model {
   public:
    // model data
    std::string name;
    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<Material> materials;
    // Owns the checkerboard texture generateFallbackModel() creates -- unlike every other Texture*
    // a Mesh holds, this one isn't registered with (and thus isn't owned by) TextureManager.
    std::unique_ptr<Texture> fallbackTexture;
    std::string directory;
    std::string modelPath;
    glm::mat4 transform = glm::mat4(0.f);

    float min[3] = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity()};
    float max[3] = {-std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity()};

    Model();

    Model(Model& other);

    Model(std::vector<MeshVertex> vertices);
    Model(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices);
    Model(std::vector<MeshVertex> vertices, std::vector<unsigned int> indices, Texture* texture);
    Model(std::vector<MeshVertex> vertices, std::vector<unsigned int> indices,
          std::vector<Texture*> textures);
    ~Model();
    void render(ShaderProgram* shader, int firstFreeTextureId, bool bindTextures = true);
    void destructMesh();
    void useTriangleStrips(bool value);
    void calculatRenderData();
    void generateFallbackModel();
    std::vector<MeshVertex> getMeshVertices() {
        std::vector<MeshVertex> results;
        for (auto& mesh : meshes) {
            results.insert(results.end(), mesh->vertices.begin(), mesh->vertices.end());
        }
        return results;
    }
    std::vector<unsigned int> getIndices() {
        std::vector<unsigned int> results;
        for (auto& mesh : meshes) {
            results.insert(results.end(), mesh->indices.begin(), mesh->indices.end());
        }
        return results;
    }
    GeometricData calculateGeometricData();
    glm::vec3 getMaxCoordinates();
    glm::vec3 getMinCoordinates();
    static size_t getNextID();
    void setMesh(std::unique_ptr<Mesh> mesh) { meshes.push_back(std::move(mesh)); }

   private:
    void createMesh(std::vector<MeshVertex>& vertices, std::vector<unsigned int>& indices,
                    std::vector<Texture*>& textures);
    unsigned int TextureFromFile(const char* path, const std::string& directory,
                                 bool gamma = false);
    glm::vec3 calculateCentroid(const std::vector<std::unique_ptr<Mesh>>& meshes);
    glm::mat3 calculatePrincipalAxes(const std::vector<std::unique_ptr<Mesh>>& meshes);
};
