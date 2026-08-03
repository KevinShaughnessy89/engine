#pragma once
#include "MeshStructs.hpp"
#include "core/PrecompiledHeader.hpp"

class Texture;
class ShaderProgram;

class Mesh {
   public:
    Mesh();
    ~Mesh();
    Mesh(std::vector<MeshVertex>&& vertices);
    Mesh(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices);
    Mesh(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices,
         std::vector<Texture*>&& textures);
    Mesh(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices, Texture* texture);
    Mesh(Mesh& other) {
        this->VAO = other.VAO;
        this->VBO = other.VBO;
        this->EBO = other.EBO;
        this->vertices = other.vertices;
        this->indices = other.indices;
        this->textures = other.textures;
        this->materialIndex = other.materialIndex;
        this->meshSetup = other.meshSetup;
        this->hasNormalMap = other.hasNormalMap;
        this->hasSpecularMap = other.hasSpecularMap;
        this->hasEmissiveMap = other.hasEmissiveMap;
        this->hasShininessMap = other.hasShininessMap;
    }
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept
        : VAO(other.VAO),
          VBO(other.VBO),
          EBO(other.EBO),
          vertices(std::move(other.vertices)),
          indices(std::move(other.indices)),
          textures(std::move(other.textures)),
          materialIndex(other.materialIndex),
          meshSetup(other.meshSetup),
          hasNormalMap(other.hasNormalMap),
          hasSpecularMap(other.hasSpecularMap),
          hasEmissiveMap(other.hasEmissiveMap),
          hasShininessMap(other.hasShininessMap) {
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
    }

    Mesh& operator=(Mesh&& other) noexcept {
        if (&other != this) {
            cleanUp();

            VAO = other.VAO;
            VBO = other.VBO;
            EBO = other.EBO;

            vertices = std::move(other.vertices);
            indices = std::move(other.indices);
            textures = std::move(other.textures);
            materialIndex = other.materialIndex;
            meshSetup = other.meshSetup;
            hasNormalMap = other.hasNormalMap;
            hasSpecularMap = other.hasSpecularMap;
            hasEmissiveMap = other.hasEmissiveMap;
            hasShininessMap = other.hasShininessMap;

            // Reset other so it won't delete our resources
            other.VAO = 0;
            other.VBO = 0;
            other.EBO = 0;
            other.meshSetup = false;
        }
        return *this;
    }

    void addVertex(glm::vec3 position, glm::vec3 normal, glm::vec2 texCoords, glm::vec3 tangent,
                   glm::vec3 bitangent);
    void addIndices(const std::vector<unsigned int>& indices);
    void addVertexAttribute(GLuint index, GLint size, GLenum type, GLboolean normalized,
                            GLsizei stride, const void* pointer);
    void setupMesh();
    void setQuad();
    void setCube();
    void render(ShaderProgram* shader, int firstFreeTextureId = 0, bool bindTextures = true);
    void setUseTriangleStrips(bool value) { this->useTriangleStrips = value; }
    static bool hasNormalMapTexture(const std::vector<Texture*>& textures);
    static bool hasAlphaMapTexture(const std::vector<Texture*>& textures);
    static bool hasSpecularMapTexture(const std::vector<Texture*>& textures);
    static bool hasEmissiveMapTexture(const std::vector<Texture*>& textures);
    static bool hasShininessMapTexture(const std::vector<Texture*>& textures);
    std::vector<glm::vec4> getPositions();
    std::vector<glm::vec3> getNormals();
    std::vector<glm::vec2> getTexCoords();
    std::vector<glm::vec3> getTangents();
    std::vector<glm::vec3> getBitangents();

    GLuint VAO, VBO, EBO;
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<VertexAttribute> attributes;
    std::vector<Texture*> textures;
    int materialIndex = -1;
    bool meshSetup = false;
    bool useTriangleStrips = false;
    bool hasNormalMap = false;
    bool hasSpecularMap = false;
    bool hasEmissiveMap = false;
    bool hasShininessMap = false;

    void cleanUp();
};
