#include "rendering/LightSource.hpp"

#include "core/PrecompiledHeader.hpp"
#include "rendering/ShaderProgram.hpp"

DirectionalLight LightSource::createDirectionalStruct(const LightSourceState& light) {
    return DirectionalLight(light.direction, light.ambient, light.diffuse, light.specular);
}

PointLight LightSource::createPointLightStruct(const LightSourceState& light) {
    return PointLight(light.position, light.constant, light.linear, light.quadratic, light.ambient,
                      light.diffuse, light.specular);
}

SpotLight LightSource::createSpotLightStruct(const LightSourceState& light) {
    return SpotLight(light.position, light.constant, light.linear, light.quadratic, light.direction,
                     light.ambient, light.diffuse, light.specular, light.cutOff, light.outerCutOff);
}

void LightSource::setDirectionalShaders(const LightSourceState& light, ShaderProgram* shader) {
    shader->setVec3("DirLight.direction", light.direction);
    shader->setVec3("DirLight.ambient", light.ambient);
    shader->setVec3("DirLight.diffuse", light.diffuse);
    shader->setVec3("DirLight.specular", light.specular);
}

void LightSource::setPointShaders(const LightSourceState& light, ShaderProgram* shader, int index) {
    std::string base = "pointLights[" + std::to_string(index) + "].";
    shader->setVec3(base + "position", light.position);
    shader->setVec3(base + "ambient", light.ambient);
    shader->setVec3(base + "diffuse", light.diffuse);
    shader->setVec3(base + "specular", light.specular);
    shader->setFloat(base + "constant", light.constant);
    shader->setFloat(base + "linear", light.linear);
    shader->setFloat(base + "quadratic", light.quadratic);
}

void LightSource::setSpotShaders(const LightSourceState& light, ShaderProgram* shader) {
    shader->setVec3("spotLight.position", light.position);
    shader->setVec3("spotLight.direction", light.direction);
    shader->setVec3("spotLight.ambient", light.ambient);
    shader->setVec3("spotLight.diffuse", light.diffuse);
    shader->setVec3("spotLight.specular", light.specular);
    shader->setFloat("spotLight.constant", light.constant);
    shader->setFloat("spotLight.linear", light.linear);
    shader->setFloat("spotLight.quadratic", light.quadratic);
    shader->setFloat("spotLight.cutOff", light.cutOff);
    shader->setFloat("spotLight.outerCutOff", light.outerCutOff);
}