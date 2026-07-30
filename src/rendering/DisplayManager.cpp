#include "DisplayManager.hpp"

#include "ComputeShader.hpp"
#include "Model.hpp"
#include "ShaderProgram.hpp"
#include "core/Config.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/Renderer.hpp"


DisplayManager::DisplayManager() {
    orthographicMatrix = glm::ortho(0.0f, (float)Config::SCREEN_WIDTH, 0.0f,
                                    (float)Config::SCREEN_HEIGHT, -1.0f, 1.0f);
    DisplayState::perspectiveMatrix =
        glm::perspective(glm::radians(DisplayState::perspectiveFOV), DisplayState::aspect,
                         DisplayState::nearPlane, DisplayState::farPlane);
}

DisplayManager::~DisplayManager() {
    delete modelShader;
    delete lightShader;
    delete terrainShader;
}

void DisplayManager::init() {
    renderer.init();
}

void DisplayManager::updateProjectionMatrices(int screenWidth, int screenHeight) {
    orthographicMatrix =
        glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);
    DisplayState::perspectiveMatrix =
        glm::perspective(glm::radians(DisplayState::perspectiveFOV), DisplayState::aspect,
                         DisplayState::nearPlane, DisplayState::farPlane);
}

void DisplayManager::setAspectValues(float frameBufferWidth, float frameBufferHeight) {
    DisplayState::screenWidth = frameBufferWidth;
    DisplayState::screenHeight = frameBufferHeight;
    DisplayState::aspect = frameBufferWidth / frameBufferHeight;
    updateProjectionMatrices(DisplayState::screenWidth, DisplayState::screenHeight);
}

void DisplayManager::loadShaders() {
    modelShader = new ShaderProgram(Config::modelVertexPath, Config::modelFragmentPath);
    animatedModelShader =
        new ShaderProgram(Config::animatedModelVertexPath, Config::modelFragmentPath);
    lightShader = new ShaderProgram(Config::lightVertexPath, Config::lightFragmentPath);
    terrainShader = new ShaderProgram(Config::terrainVertexPath, Config::terrainFragmentPath,
                                      Config::terrainTessControlPath,
                                      Config::terrainTessEvaluationPath);
    skyShader = new ShaderProgram(Config::skyVertexPath, Config::skyFragmentPath);
    particleShader = new ShaderProgram(Config::particleVertexPath, Config::particleFragmentPath);
    combineShader = new ShaderProgram(Config::combineVertexPath, Config::combineFragmentPath);
    blurShader = new ShaderProgram(Config::blurVertexPath, Config::blurFragmentPath);
    sunShader = new ShaderProgram(Config::sunVertexPath, Config::sunFragmentPath);
    shadowDepthShader =
        new ShaderProgram(Config::shadowDepthVertexPath, Config::shadowDepthFragmentPath);
    instancedShadowDepthShader = new ShaderProgram(Config::instancedShadowDepthVertexPath,
                                                   Config::shadowDepthFragmentPath);
    instancedModelShader =
        new ShaderProgram(Config::instancedModelVertexPath, Config::instancedModelFragmentPath);
    imposterModelShader =
        new ShaderProgram(Config::imposterModelVertexPath, Config::imposterModelFragmentPath);
    irradianceShader =
        new ShaderProgram(Config::irradianceVertexPath, Config::irradianceFragmentPath);
    normalMapShader = new ComputeShader(Config::computeShaderPath);
}

ShaderProgram* DisplayManager::getSkyShader() {
    return skyShader;
}

ShaderProgram* DisplayManager::getModelShader() {
    return modelShader;
}

ShaderProgram* DisplayManager::getLightShader() {
    return lightShader;
}

ShaderProgram* DisplayManager::getTerrainShader() {
    return terrainShader;
}

ShaderProgram* DisplayManager::getParticleShader() {
    return particleShader;
}

void DisplayManager::useModelShader() {
    modelShader->use();
}

void DisplayManager::useLightShader() {
    lightShader->use();
}

void DisplayManager::useBlurShader() {
    blurShader->use();
}

void DisplayManager::useCombineShader() {
    combineShader->use();
}

void DisplayManager::useTerrainShader() {
    terrainShader->use();
}

void DisplayManager::useSkyShader() {
    skyShader->use();
}

void DisplayManager::useParticleShader() {
    particleShader->use();
}

void DisplayManager::setDummyInverseTranspose() {
    modelShader->setMat3("inverseTranspose", glm::mat3(1.0f));
}

glm::mat4 DisplayManager::getShaderViewMatrix() {
    GLint uniformLocation = glGetUniformLocation(modelShader->ID, "view");

    if (uniformLocation == -1) {
        std::cerr << "Uniform " << "view" << " not found in shader." << std::endl;
        return glm::mat4(1.0f);
    }

    GLfloat matrixArray[16];
    glGetUniformfv(modelShader->ID, uniformLocation, matrixArray);
    return glm::make_mat4(matrixArray);
}

void DisplayManager::setOrthographicProjection() {
    modelShader->setMat4("projection", this->orthographicMatrix);
}

void DisplayManager::setPerspectiveProjection() {
    modelShader->setMat4("projection", DisplayState::perspectiveMatrix);
}

void DisplayManager::render(double currentTime) {
    Display.getRenderer().renderBatches(currentTime);
}

ShaderProgram* DisplayManager::getShaderProgramFromID(GLuint id) {
    if (modelShader && id == modelShader->ID) return modelShader;
    if (animatedModelShader && id == animatedModelShader->ID) return animatedModelShader;
    if (lightShader && id == lightShader->ID) return lightShader;
    if (terrainShader && id == terrainShader->ID) return terrainShader;
    if (skyShader && id == skyShader->ID) return skyShader;
    if (particleShader && id == particleShader->ID) return particleShader;
    if (sunShader && id == sunShader->ID) return sunShader;
    if (instancedModelShader && id == instancedModelShader->ID) return instancedModelShader;

    return nullptr;
}

GLuint DisplayManager::getModelShaderID() {
    return modelShader->ID;
};
GLuint DisplayManager::getAnimatedModelShaderID() {
    return animatedModelShader->ID;
};
GLuint DisplayManager::getLightShaderID() {
    return lightShader->ID;
};
GLuint DisplayManager::getTerrainShaderID() {
    return terrainShader->ID;
};
GLuint DisplayManager::getSkyShaderID() {
    return skyShader->ID;
};
GLuint DisplayManager::getParticleShaderID() {
    return particleShader->ID;
};
GLuint DisplayManager::getSunShaderID() {
    return sunShader->ID;
};
GLuint DisplayManager::getInstancedModelShaderID() {
    return instancedModelShader->ID;
};
GLuint DisplayManager::getImposterModelShaderID() {
    return imposterModelShader->ID;
};

ShaderProgram* DisplayManager::getShaderProgramFromType(ShaderType shaderType) {
    switch (shaderType) {
        case ShaderType::Model:
            return modelShader;
        case ShaderType::AnimatedModel:
            return animatedModelShader;
        case ShaderType::Light:
            return lightShader;
        case ShaderType::Terrain:
            return terrainShader;
        case ShaderType::Sky:
            return skyShader;
        case ShaderType::Particle:
            return particleShader;
        case ShaderType::Blur:
            return blurShader;
        case ShaderType::Combine:
            return combineShader;
        case ShaderType::Sun:
            return sunShader;
        case ShaderType::NormalMap:
            return nullptr;
        case ShaderType::ShadowDepth:
            return shadowDepthShader;
        case ShaderType::InstancedShadowDepth:
            return instancedShadowDepthShader;
        case ShaderType::InstancedModel:
            return instancedModelShader;
        case ShaderType::ImposterModel:
            return imposterModelShader;
    }

    return nullptr;
};

GLuint DisplayManager::getShaderIdFromShaderType(ShaderType shaderType) {
    switch (shaderType) {
        case ShaderType::Model:
            return modelShader->ID;
        case ShaderType::AnimatedModel:
            return animatedModelShader->ID;
        case ShaderType::Light:
            return lightShader->ID;
        case ShaderType::Terrain:
            return terrainShader->ID;
        case ShaderType::Sky:
            return skyShader->ID;
        case ShaderType::Particle:
            return particleShader->ID;
        case ShaderType::Blur:
            return blurShader->ID;
        case ShaderType::Combine:
            return combineShader->ID;
        case ShaderType::Sun:
            return sunShader->ID;
        case ShaderType::NormalMap:
            return normalMapShader->ID;
        case ShaderType::ShadowDepth:
            return shadowDepthShader->ID;
        case ShaderType::InstancedShadowDepth:
            return instancedShadowDepthShader->ID;
        case ShaderType::InstancedModel:
            return instancedModelShader->ID;
        case ShaderType::ImposterModel:
            return imposterModelShader->ID;
    }

    return 0;
};
