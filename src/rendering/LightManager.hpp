#pragma once

#include "LightType.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entity/fwd.hpp"
#include "rendering/LightSource.hpp"

class ShaderProgram;

class LightManager {
   public:
    LightManager();
    GLuint directionalLightUBO;
    GLuint pointLightUBO;
    GLuint spotLightUBO;
    static int numDirLights;
    static int numPointLights;
    static int numSpotLights;
    void printUBOShaderData();
    std::vector<entt::entity> LightSourceRegistry;
    std::vector<entt::entity> DirectionalStructRegistry;
    std::vector<entt::entity> PointStructRegistry;
    std::vector<entt::entity> SpotStructRegistry;
    void Draw(ShaderProgram* shader);
    void addLightSource(entt::entity light);
    void initUBOs();
    void updateUBOs();

   private:
};