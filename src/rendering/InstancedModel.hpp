#pragma once
#include "InstancedMesh.hpp"
#include "Material.hpp"

class Texture;
class ShaderProgram;
struct GeometricData;

class InstancedModel {
   public:
    InstancedModel();
    InstancedModel(std::vector<MeshVertex>&& vertices);
    InstancedModel(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices);
    InstancedModel(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices,
                   Texture* texture);
    InstancedModel(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices,
                   std::vector<Texture*>&& textures);

    std::string name;
    std::string directory;
    std::string modelPath;
    glm::mat4 transform = glm::mat4(0.f);

    float min[3] = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity()};
    float max[3] = {-std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity()};

    std::vector<InstancedMesh> meshes;
    std::vector<Material> materials;
    void updateBufferData(std::vector<InstancedMeshVertex>&& bufferData);
    void render(ShaderProgram* shader, int firstFreeTextureID);
    glm::vec3 getMaxCoordinates();
    glm::vec3 getMinCoordinates();
    GeometricData calculateGeometricData();
    GeometricData calculateGeometricDataForMaterial(const std::string& material);

    // Full, unculled set of instance placements (e.g. every tree in the world). Renderer
    // frustum-culls this each frame and pushes only the visible subset into meshes via
    // updateVisibleInstances(), rather than submitting every instance to the GPU every frame.
    std::vector<InstancedMeshVertex> allInstances;
    void setInstances(std::vector<InstancedMeshVertex>&& instances);
    void updateVisibleInstances(const std::vector<InstancedMeshVertex>& visibleInstances);
    void addTexture(std::string name, std::string uniform);
};
