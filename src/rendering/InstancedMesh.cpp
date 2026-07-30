#include "InstancedMesh.hpp"

#include <tracy/Tracy.hpp>

#include "ShaderProgram.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/Texture.hpp"

InstancedMesh::InstancedMesh(std::vector<MeshVertex>&& vertices) {
    this->vertices = std::move(vertices);
    setupMesh();
}

InstancedMesh::InstancedMesh(std::vector<MeshVertex>&& vertices,
                             std::vector<unsigned int>&& indices) {
    this->vertices = std::move(vertices);
    this->indices = std::move(indices);
    setupMesh();
}

InstancedMesh::InstancedMesh(std::vector<MeshVertex>&& vertices,
                             std::vector<unsigned int>&& indices,
                             std::vector<Texture*>&& textures) {
    this->vertices = std::move(vertices);
    this->indices = std::move(indices);
    this->textures = std::move(textures);
    hasNormalMap = hasNormalMapTexture(this->textures);
    hasAlphaMap = hasAlphaMapTexture(this->textures);
    setupMesh();
}

InstancedMesh::InstancedMesh(std::vector<MeshVertex>&& vertices,
                             std::vector<unsigned int>&& indices, Texture* texture) {
    this->vertices = std::move(vertices);
    this->indices = std::move(indices);
    this->textures.push_back(texture);
    hasNormalMap = hasNormalMapTexture(this->textures);
    hasAlphaMap = hasAlphaMapTexture(this->textures);
    setupMesh();
}

void InstancedMesh::setupMesh() {
    if (meshSetup) {
        std::cout << "Mesh already set up, skipping setup." << std::endl;
        return;
    }

    cleanUp();
    glGenVertexArrays(1, &VAO);

    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "Error before glBindVertexArray" << std::endl;
    }

    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(MeshVertex), vertices.data(),
                 GL_STATIC_DRAW);

    if (!indices.empty()) {
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(),
                     GL_STATIC_DRAW);
    }

    // position coords
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          (void*)offsetof(MeshVertex, position));

    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          (void*)offsetof(MeshVertex, normal));

    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          (void*)offsetof(MeshVertex, tangent));

    glEnableVertexAttribArray(9);
    glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          (void*)offsetof(MeshVertex, bitangent));

    // texture coordinates
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          (void*)offsetof(MeshVertex, texCoords));

    // Set up instanced attributes
    glGenBuffers(1, &instancedVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instancedVBO);
    glBufferData(GL_ARRAY_BUFFER, maxInstances * sizeof(InstancedMeshVertex), NULL, GL_STREAM_DRAW);

    // position (instanced)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(InstancedMeshVertex),
                          (void*)offsetof(InstancedMeshVertex, position));
    glVertexAttribDivisor(2, 1);

    // size (instanced)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(InstancedMeshVertex),
                          (void*)offsetof(InstancedMeshVertex, size));
    glVertexAttribDivisor(3, 1);

    // color (instanced)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(InstancedMeshVertex),
                          (void*)offsetof(InstancedMeshVertex, color));
    glVertexAttribDivisor(4, 1);

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(InstancedMeshVertex),
                          (void*)offsetof(InstancedMeshVertex, skew));
    glVertexAttribDivisor(5, 1);

    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(InstancedMeshVertex),
                          (void*)offsetof(InstancedMeshVertex, rotation));
    glVertexAttribDivisor(6, 1);

    meshSetup = true;
}

void InstancedMesh::uploadInstanceData() {
    ZoneScoped;
    ZoneValue(instancedBufferData.size());
    if (!meshSetup) {
        setupMesh();
    }

    glBindBuffer(GL_ARRAY_BUFFER, instancedVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, instancedBufferData.size() * sizeof(InstancedMeshVertex),
                    instancedBufferData.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void InstancedMesh::render(ShaderProgram* shader, int firstFreeTextureId, bool bindTextures) {
    ZoneScoped;
    if (!meshSetup) {
        setupMesh();
    }

    if (bindTextures) {
        for (unsigned int i = 0; i < textures.size(); i++) {
            textures[i]->bindTexture(shader, firstFreeTextureId + i);
        }
    }

    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0,
                            instancedBufferData.size());
    glBindVertexArray(0);
}
