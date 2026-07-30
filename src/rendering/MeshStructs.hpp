#pragma once
#include "core/PrecompiledHeader.hpp"

struct MeshVertex {
    glm::vec4 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::ivec4 boneIDs;
    glm::vec4 boneWeights;

    MeshVertex() : position(0.0f), normal(0.0f), texCoords(0.0f), tangent(0.0f), bitangent(0.0f) {}
    MeshVertex(const glm::vec4& pos, const glm::vec3& norm, const glm::vec2& tex,
               const glm::vec3& tan, const glm::vec3& bitan)
        : position(pos), normal(norm), texCoords(tex), tangent(tan), bitangent(bitan) {}

    const glm::vec4& getPosition() const { return position; }
    const glm::vec3& getNormal() const { return normal; }
    const glm::vec2& getTexCoords() const { return texCoords; }
    const glm::vec3& getTangent() const { return tangent; }
    const glm::vec3& getBitangent() const { return bitangent; }
};

struct VertexAttribute {
    GLuint index;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    const void* pointer;
};
