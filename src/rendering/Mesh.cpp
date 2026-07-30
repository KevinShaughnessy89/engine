#include "Mesh.hpp"

#include <thread>
#include <tracy/Tracy.hpp>

#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/ShaderProgram.hpp"
#include "rendering/Texture.hpp"
#include "rendering/TextureManager.hpp"

Mesh::~Mesh() {
    cleanUp();
}

Mesh::Mesh() : VAO(0), VBO(0), EBO(0), meshSetup(false) {
}

Mesh::Mesh(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices,
           std::vector<Texture*>&& textures)
    : vertices(std::move(vertices)),
      indices(std::move(indices)),
      textures(std::move(textures)),
      VAO(0),
      VBO(0),
      EBO(0),
      meshSetup(false) {
    hasNormalMap = hasNormalMapTexture(this->textures);
    hasAlphaMapTexture(this->textures);
    if (std::any_of(this->textures.begin(), this->textures.end(),
                    [](Texture* t) { return t == nullptr; })) {
        std::cout << "Warning: texture is null or textures vector is empty." << std::endl;
    } else {
        setupMesh();
    }
}

Mesh::Mesh(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices,
           Texture* texture)
    : vertices(std::move(vertices)),
      indices(std::move(indices)),
      VAO(0),
      VBO(0),
      EBO(0),
      meshSetup(false) {
    this->textures.push_back(texture);
    hasNormalMap = hasNormalMapTexture(this->textures);
    hasAlphaMapTexture(this->textures);
    setupMesh();
}

bool Mesh::hasNormalMapTexture(const std::vector<Texture*>& textures) {
    return std::any_of(textures.begin(), textures.end(),
                       [](Texture* t) { return t && t->uniform == "textureNormal"; });
}

bool Mesh::hasAlphaMapTexture(const std::vector<Texture*>& textures) {
    return std::any_of(textures.begin(), textures.end(),
                       [](Texture* t) { return t && t->uniform == "textureAlpha"; });
}

Mesh::Mesh(std::vector<MeshVertex>&& vertices)
    : vertices(std::move(vertices)), VAO(0), VBO(0), EBO(0), meshSetup(false) {
    setupMesh();
}

Mesh::Mesh(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices)
    : vertices(std::move(vertices)),
      indices(std::move(indices)),
      VAO(0),
      VBO(0),
      EBO(0),
      meshSetup(false) {
    setupMesh();
}

void Mesh::addVertex(glm::vec3 position, glm::vec3 normal, glm::vec2 texCoords, glm::vec3 tangent,
                     glm::vec3 bitangent) {
    vertices.emplace_back(glm::vec4(position, 1.0f), normal, texCoords, tangent, bitangent);
    meshSetup = false;
}

void Mesh::addIndices(const std::vector<unsigned int>& newIndices) {
    indices = newIndices;
    meshSetup = false;
}

void Mesh::addVertexAttribute(GLuint index, GLint size, GLenum type, GLboolean normalized,
                              GLsizei stride, const void* pointer) {
    VertexAttribute attribute = {index, size, type, normalized, stride, pointer};
    attributes.push_back(attribute);
    meshSetup = false;
}

void Mesh::setQuad() {
    addVertex(glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f),
              glm::vec3(0.0f));
    addVertex(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f),
              glm::vec3(0.0f));
    addVertex(glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f),
              glm::vec3(0.0f));
    addVertex(glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 0.0f),
              glm::vec3(0.0f), glm::vec3(0.0f));

    static std::vector<unsigned int> quadIndices = {
        0, 1, 2,  // first triangle
        2, 3, 0   // second triangle
    };
    addIndices(quadIndices);

    setupMesh();

    std::cout << "setupMesh running on thread " << std::this_thread::get_id() << std::endl;
}

void Mesh::setCube() {
    addVertex(glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f),
              glm::vec3(0.0f));
    addVertex(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f),
              glm::vec3(0.0f));

    addVertex(glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f),
              glm::vec3(0.0f));
    addVertex(glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 0.0f),
              glm::vec3(0.0f), glm::vec3(0.0f));

    addVertex(glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f), glm::vec2(1.0f, 0.0f),
              glm::vec3(0.0f), glm::vec3(0.0f));
    addVertex(glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f),
              glm::vec3(0.0f));

    addVertex(glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f), glm::vec2(0.0f, 0.0f),
              glm::vec3(0.0f), glm::vec3(0.0f));
    addVertex(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f), glm::vec2(1.0f, 0.0f),
              glm::vec3(0.0f), glm::vec3(0.0f));

    static std::vector<unsigned int> cubeIndices = {

        // front (0,1,2,3)
        0, 2, 1, 2, 0, 3,

        // back (4,5,6,7)
        4, 5, 6, 6, 7, 4,

        // top (0,1,5,4)
        0, 1, 5, 5, 4, 0,

        // bottom (3,2,6,7)
        3, 6, 2, 6, 3, 7,

        // left (0,3,7,4)
        0, 7, 3, 7, 0, 4,

        // right (1,2,6,5)
        1, 2, 6, 6, 5, 1};

    addIndices(cubeIndices);

    setupMesh();
}

void Mesh::setupMesh() {
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

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          (void*)offsetof(MeshVertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          (void*)offsetof(MeshVertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          (void*)offsetof(MeshVertex, texCoords));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          (void*)offsetof(MeshVertex, tangent));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          (void*)offsetof(MeshVertex, bitangent));

    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, sizeof(MeshVertex), (void*)offsetof(MeshVertex, boneIDs));

    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          (void*)offsetof(MeshVertex, boneWeights));

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL Error after VAO creation: " << err << std::endl;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    meshSetup = true;
}

void Mesh::render(ShaderProgram* shader, int firstFreeTextureId, bool bindTextures) {
    ZoneScoped;
    bool toggleDebug = false;

    if (meshSetup) {
        // Depth-only passes (e.g. the shadow map) bind a shader with no sampler uniforms at all,
        // so binding textures there is both wasted work and a spurious "uniform doesn't exist"
        // warning from ShaderProgram::getUniformLocation.
        if (bindTextures) {
            for (unsigned int i = 0; i < textures.size(); i++) {
                textures[i]->bindTexture(shader, firstFreeTextureId + i);
            }
        }

        glBindVertexArray(VAO);

        if (indices.size() > 0) {
            if (!this->useTriangleStrips) {
                glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()),
                               GL_UNSIGNED_INT, 0);
            } else {
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<unsigned int>(indices.size()),
                               GL_UNSIGNED_INT, 0);
            }
        } else {
            glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        }

        if (toggleDebug) {
            std::cout << "VAO: " << VAO << ", VBO: " << VBO << ", EBO: " << EBO << std::endl;
            std::cout << "glIsVertexArray(VAO): " << static_cast<bool>(glIsVertexArray(VAO))
                      << std::endl;
            std::cout << "glIsBuffer(VBO): " << static_cast<bool>(glIsBuffer(VBO)) << std::endl;
            std::cout << "glIsBuffer(EBO): " << static_cast<bool>(glIsBuffer(EBO)) << std::endl;

            std::cout << "First 5 vertices:" << std::endl;
            for (size_t i = 0; i < std::min(vertices.size(), size_t(5)); ++i) {
                const MeshVertex& v = vertices[i];
                std::cout << "Vertex " << i << ": " << "Pos(" << v.position.x << ", "
                          << v.position.y << ", " << v.position.z << ", " << v.position.w << "), "
                          << "Norm(" << v.normal.x << ", " << v.normal.y << ", " << v.normal.z
                          << "), " << "Tex(" << v.texCoords.x << ", " << v.texCoords.y << "), "
                          << "Tan(" << v.tangent.x << ", " << v.tangent.y << ", " << v.tangent.z
                          << "), " << "Bitan(" << v.bitangent.x << ", " << v.bitangent.y << ", "
                          << v.bitangent.z << ")" << std::endl;
            }

            std::cout << "First 10 indices:" << std::endl;
            for (size_t i = 0; i < std::min(indices.size(), size_t(10)); ++i) {
                std::cout << indices[i] << " ";
            }
            std::cout << std::endl;
        }

        GLenum err = glGetError();
        if (err != GL_NO_ERROR && toggleDebug) {
            std::cerr << "GL Error after draw call: " << err << std::endl;

            GLint currentProgram = 0;
            glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
            std::cout << "Active shader program: " << currentProgram << std::endl;

            GLint currentVAO = 0;
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
            std::cout << "Bound VAO: " << currentVAO << std::endl;

            GLint currentVBO = 0;
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &currentVBO);
            std::cout << "Bound VBO: " << currentVBO << std::endl;

            GLint currentEBO = 0;
            glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &currentEBO);
            std::cout << "Bound EBO: " << currentEBO << std::endl;

            GLint boundTex = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
            std::cout << "Texture bound: " << boundTex << std::endl;

            GLint activeTexUnit = 0;
            glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexUnit);
            std::cout << "Active Texture Unit: GL_TEXTURE" << (activeTexUnit - GL_TEXTURE0)
                      << std::endl;

            GLint loc = glGetUniformLocation(currentProgram, "terrainTexture");
            std::cout << "Uniform 'terrainTexture' location: " << loc << std::endl;
            if (loc == -1) {
                std::cerr << "WARNING: Uniform 'terrainTexture' is not active or not found."
                          << std::endl;
            }

            for (int i = 0; i < 5; ++i) {
                GLboolean enabled;
                glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, (GLint*)&enabled);
                std::cout << "Attribute " << i << " enabled: " << (enabled ? "Yes" : "No")
                          << std::endl;
            }
        }

        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    } else {
        if (toggleDebug) std::cerr << "Attempting to render uninitialized mesh." << std::endl;
    }
}

// Implementation (for Mesh.cpp)
std::vector<glm::vec4> Mesh::getPositions() {
    std::vector<glm::vec4> positions;
    positions.reserve(vertices.size());
    for (const auto& vertex : vertices) {
        positions.push_back(vertex.getPosition());
    }
    return positions;
}

std::vector<glm::vec3> Mesh::getNormals() {
    std::vector<glm::vec3> normals;
    normals.reserve(vertices.size());
    for (const auto& vertex : vertices) {
        normals.push_back(vertex.getNormal());
    }
    return normals;
}

std::vector<glm::vec2> Mesh::getTexCoords() {
    std::vector<glm::vec2> texCoords;
    texCoords.reserve(vertices.size());
    for (const auto& vertex : vertices) {
        texCoords.push_back(vertex.getTexCoords());
    }
    return texCoords;
}

std::vector<glm::vec3> Mesh::getTangents() {
    std::vector<glm::vec3> tangents;
    tangents.reserve(vertices.size());
    for (const auto& vertex : vertices) {
        tangents.push_back(vertex.getTangent());
    }
    return tangents;
}

std::vector<glm::vec3> Mesh::getBitangents() {
    std::vector<glm::vec3> bitangents;
    bitangents.reserve(vertices.size());
    for (const auto& vertex : vertices) {
        bitangents.push_back(vertex.getBitangent());
    }
    return bitangents;
}

void Mesh::cleanUp() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
    meshSetup = false;
}
