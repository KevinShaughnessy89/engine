#include "rendering/LightManager.hpp"

#include "LightSource.hpp"
#include "components/rendering/LightSourceState.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/LightSource.hpp"

int LightManager::numDirLights = 0;
int LightManager::numPointLights = 0;
int LightManager::numSpotLights = 0;

LightManager::LightManager() {
}

void LightManager::addLightSource(entt::entity light) {
    LightSourceState& lightState = Registry.get<LightSourceState>(light);

    if (lightState.type == LightType::DIRECTIONAL)
        DirectionalStructRegistry.push_back(light);
    else if (lightState.type == LightType::POINT)
        PointStructRegistry.push_back(light);
    else if (lightState.type == LightType::SPOT)
        SpotStructRegistry.push_back(light);
    LightSourceRegistry.push_back(light);
    // else std::cout << "Failed to add light source object." << std::endl;
    // std::cout << "end of addLightSource() for " << light.UID << " position: " <<
    // glm::to_string(light.position) << std::endl;
}

const int MAX_POINT_LIGHTS = 10;
const int MAX_DIR_LIGHTS = 10;
const int MAX_SPOT_LIGHTS = 10;

void LightManager::initUBOs() {
    numDirLights = DirectionalStructRegistry.size();
    numPointLights = PointStructRegistry.size();
    numSpotLights = SpotStructRegistry.size();

    // Point Lights UBO
    glGenBuffers(1, &pointLightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, pointLightUBO);
    size_t pointBufferSize = sizeof(int) + MAX_POINT_LIGHTS * sizeof(PointLight);
    glBufferData(GL_UNIFORM_BUFFER, pointBufferSize, NULL, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &numPointLights);
    glBufferSubData(
        GL_UNIFORM_BUFFER, sizeof(int),
        std::min(MAX_POINT_LIGHTS, (int)PointStructRegistry.size()) * sizeof(PointLight),
        PointStructRegistry.data());
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, pointLightUBO);

    // Spot Lights UBO
    glGenBuffers(1, &spotLightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, spotLightUBO);
    size_t spotBufferSize = sizeof(int) + MAX_SPOT_LIGHTS * sizeof(SpotLight);
    glBufferData(GL_UNIFORM_BUFFER, spotBufferSize, NULL, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &numSpotLights);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(int),
                    std::min(MAX_SPOT_LIGHTS, (int)SpotStructRegistry.size()) * sizeof(SpotLight),
                    SpotStructRegistry.data());
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, spotLightUBO);
    // Directional Lights UBO
    glGenBuffers(1, &directionalLightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, directionalLightUBO);
    size_t directionalBufferSize = sizeof(int) + MAX_DIR_LIGHTS * sizeof(DirectionalLight);
    glBufferData(GL_UNIFORM_BUFFER, directionalBufferSize, NULL, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &numDirLights);
    glBufferSubData(
        GL_UNIFORM_BUFFER, sizeof(int),
        std::min(MAX_DIR_LIGHTS, (int)DirectionalStructRegistry.size()) * sizeof(DirectionalLight),
        DirectionalStructRegistry.data());
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, directionalLightUBO);

    // WprintUBOShaderData();
}

void LightManager::updateUBOs() {
    // Update Point Lights
    glBindBuffer(GL_UNIFORM_BUFFER, pointLightUBO);
    numPointLights = std::min(MAX_POINT_LIGHTS, (int)PointStructRegistry.size());
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &numPointLights);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(int), numPointLights * sizeof(PointLight),
                    PointStructRegistry.data());

    // Update Spot Lights
    glBindBuffer(GL_UNIFORM_BUFFER, spotLightUBO);
    numSpotLights = std::min(MAX_SPOT_LIGHTS, (int)SpotStructRegistry.size());
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &numSpotLights);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(int), numSpotLights * sizeof(SpotLight),
                    SpotStructRegistry.data());

    // Update Directional Lights
    glBindBuffer(GL_UNIFORM_BUFFER, directionalLightUBO);
    numDirLights = std::min(MAX_DIR_LIGHTS, (int)DirectionalStructRegistry.size());
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &numDirLights);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(int), numDirLights * sizeof(DirectionalLight),
                    DirectionalStructRegistry.data());
}

void LightManager::printUBOShaderData() {
    glBindBuffer(GL_UNIFORM_BUFFER, pointLightUBO);
    PointLight* pointLightData = (PointLight*)glMapBuffer(GL_UNIFORM_BUFFER, GL_READ_ONLY);
    if (pointLightData) {
        for (int i = 0; i < numPointLights; ++i) {
            std::cout << "Point Light " << i << std::endl;
            std::cout << "Position: " << pointLightData[i].position.x << ", "
                      << pointLightData[i].position.y << ", " << pointLightData[i].position.z
                      << std::endl;
            std::cout << "Constant: " << pointLightData[i].constant << std::endl;
            std::cout << "Linear: " << pointLightData[i].linear << std::endl;
            std::cout << "Quadratic: " << pointLightData[i].quadratic << std::endl;
            std::cout << "Ambient: " << pointLightData[i].ambient.x << ", "
                      << pointLightData[i].ambient.y << ", " << pointLightData[i].ambient.z
                      << std::endl;
            std::cout << "Diffuse: " << pointLightData[i].diffuse.x << ", "
                      << pointLightData[i].diffuse.y << ", " << pointLightData[i].diffuse.z
                      << std::endl;
            std::cout << "Specular: " << pointLightData[i].specular.x << ", "
                      << pointLightData[i].specular.y << ", " << pointLightData[i].specular.z
                      << std::endl;
        }
        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, spotLightUBO);
    SpotLight* spotLightData = (SpotLight*)glMapBuffer(GL_UNIFORM_BUFFER, GL_READ_ONLY);
    if (spotLightData) {
        for (int i = 0; i < numSpotLights; ++i) {
            std::cout << "Spot Light " << i << std::endl;
            std::cout << "Position: " << spotLightData[i].position.x << ", "
                      << spotLightData[i].position.y << ", " << spotLightData[i].position.z
                      << std::endl;
            std::cout << "Direction: " << spotLightData[i].direction.x << ", "
                      << spotLightData[i].direction.y << ", " << spotLightData[i].direction.z
                      << std::endl;
            std::cout << "CutOff: " << spotLightData[i].cutOff << std::endl;
            std::cout << "OuterCutOff: " << spotLightData[i].outerCutOff << std::endl;
            std::cout << "Constant: " << spotLightData[i].constant << std::endl;
            std::cout << "Linear: " << spotLightData[i].linear << std::endl;
            std::cout << "Quadratic: " << spotLightData[i].quadratic << std::endl;
            std::cout << "Ambient: " << spotLightData[i].ambient.x << ", "
                      << spotLightData[i].ambient.y << ", " << spotLightData[i].ambient.z
                      << std::endl;
            std::cout << "Diffuse: " << spotLightData[i].diffuse.x << ", "
                      << spotLightData[i].diffuse.y << ", " << spotLightData[i].diffuse.z
                      << std::endl;
            std::cout << "Specular: " << spotLightData[i].specular.x << ", "
                      << spotLightData[i].specular.y << ", " << spotLightData[i].specular.z
                      << std::endl;
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, directionalLightUBO);
    DirectionalLight* directionalLightData =
        (DirectionalLight*)glMapBuffer(GL_UNIFORM_BUFFER, GL_READ_ONLY);
    if (directionalLightData) {
        for (int i = 0; i < numDirLights; ++i) {
            std::cout << "Directional Light " << i << std::endl;
            std::cout << "Direction: " << directionalLightData[i].direction.x << ", "
                      << directionalLightData[i].direction.y << ", "
                      << directionalLightData[i].direction.z << std::endl;
            std::cout << "Ambient: " << directionalLightData[i].ambient.x << ", "
                      << directionalLightData[i].ambient.y << ", "
                      << directionalLightData[i].ambient.z << std::endl;
            std::cout << "Diffuse: " << directionalLightData[i].diffuse.x << ", "
                      << directionalLightData[i].diffuse.y << ", "
                      << directionalLightData[i].diffuse.z << std::endl;
            std::cout << "Specular: " << directionalLightData[i].specular.x << ", "
                      << directionalLightData[i].specular.y << ", "
                      << directionalLightData[i].specular.z << std::endl;
        }
        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }
}
