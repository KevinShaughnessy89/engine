#pragma once
#include <vector>

#include "InstancedMeshVertex.hpp"
#include "Mesh.hpp"
#include "core/PrecompiledHeader.hpp"

class Texture;
class ShaderProgram;

class InstancedMesh : public Mesh {
   public:
    InstancedMesh(std::vector<MeshVertex>&& vertices);

    // Construct with move semantics
    InstancedMesh(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices);

    InstancedMesh(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices,
                  std::vector<Texture*>&& textures);

    InstancedMesh(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices,
                  Texture* texture);

    void setupMesh();
    // Uploads instancedBufferData to the GPU -- call once per frame after culling, not per draw
    // pass, since shadow/occlusion/main all draw the same instance data via render().
    void uploadInstanceData();
    void render(ShaderProgram* shader, int firstFreeTextureId = 0, bool bindTextures = true);

    GLuint instancedVBO = 0;
    GLuint instancedVAO = 0;
    int maxInstances = 100000;
    std::string material;
    bool hasNormalMap = false;
    bool hasAlphaMap = false;

    std::vector<MeshVertex> instancedVertices;
    std::vector<InstancedMeshVertex> instancedBufferData;
};
