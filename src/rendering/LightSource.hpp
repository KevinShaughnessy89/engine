#pragma once

#include "LightType.hpp"
#include "components/rendering/LightSourceState.hpp"
#include "core/PrecompiledHeader.hpp"

class ShaderProgram;

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    DirectionalLight()
        : direction(glm::vec3(0.0f)),
          ambient(glm::vec3(0.0f)),
          diffuse(glm::vec3(0.0f)),
          specular(glm::vec3(0.0f)) {}
    DirectionalLight(glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
        : direction(direction), ambient(ambient), diffuse(diffuse), specular(specular) {}
};

struct PointLight {
    glm::vec3 position;
    float constant;
    float linear;
    float quadratic;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    PointLight()
        : position(glm::vec3(0.0f)),
          constant(0.0f),
          linear(0.0f),
          quadratic(0.0f),
          ambient(glm::vec3(0.0f)),
          diffuse(glm::vec3(0.0f)),
          specular(glm::vec3(0.0f)) {}
    PointLight(glm::vec3 position, float constant, float linear, float quadratic, glm::vec3 ambient,
               glm::vec3 diffuse, glm::vec3 specular)
        : position(position),
          constant(constant),
          linear(linear),
          quadratic(quadratic),
          ambient(ambient),
          diffuse(diffuse),
          specular(specular) {}
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    SpotLight()
        : position(glm::vec3(0.0f)),
          direction(glm::vec3(0.0f)),
          cutOff(0.0f),
          outerCutOff(0.0f),
          constant(0.0f),
          linear(0.0f),
          quadratic(0.0f),
          ambient(glm::vec3(0.0f)),
          diffuse(glm::vec3(0.0f)),
          specular(glm::vec3(0.0f)) {}
    SpotLight(glm::vec3 position, float constant, float linear, float quadratic,
              glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular,
              float cutOff, float outerCutOff)
        : position(position),
          constant(constant),
          linear(linear),
          quadratic(quadratic),
          direction(direction),
          ambient(ambient),
          diffuse(diffuse),
          specular(specular),
          cutOff(cutOff),
          outerCutOff(outerCutOff) {}
};

class LightSource {
   public:
    static DirectionalLight createDirectionalStruct(const LightSourceState& light);
    static PointLight createPointLightStruct(const LightSourceState& light);
    static SpotLight createSpotLightStruct(const LightSourceState& light);

    static void setDirectionalShaders(const LightSourceState& light, ShaderProgram* shader);
    static void setPointShaders(const LightSourceState& light, ShaderProgram* shader, int index);
    static void setSpotShaders(const LightSourceState& light, ShaderProgram* shader);
};