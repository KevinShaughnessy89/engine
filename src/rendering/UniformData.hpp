#pragma once

#include "rendering/UniformName.hpp"

using UniformValue =
    std::variant<int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat3, glm::mat4>;

struct UniformData {
    UniformName name;
    UniformValue value;
    bool isDirty;
    UniformData(UniformName name, UniformValue value) : name(name), value(value) {}
    UniformData() : name(UniformName::Model), value(0) {}
};

struct TextureUniformData {
    std::string name;
    GLuint value;
    bool isDirty;
    TextureUniformData(std::string name, GLuint textureID) : name(name), value(textureID) {}
    TextureUniformData() : name("null"), value(0) {}
};