#pragma once
#include "core/PrecompiledHeader.hpp"
#include "rendering/Renderer.hpp"
#include "rendering/DisplayState.hpp"
#include "rendering/ShaderType.hpp"

class ShaderProgram;
class Renderer;
class Model;
class ComputeShader;

class DisplayManager {
   public:
    DisplayManager();
    ~DisplayManager();
    Renderer renderer;
    ShaderProgram* getModelShader();
    ShaderProgram* getAnimatedModelShader() { return animatedModelShader; }
    ShaderProgram* getLightShader();
    ShaderProgram* getTerrainShader();
    ShaderProgram* getSkyShader();
    ShaderProgram* getParticleShader();
    Renderer& getRenderer() { return renderer; }
    void setRendererWindowHeight(int height) { renderer.setWindowHeight(height); }
    void setRendererWindowWidth(int width) { renderer.setWindowWidth(width); }
    void loadShaders();
    void useModelShader();
    void useLightShader();
    void useTerrainShader();
    void useSkyShader();
    void useParticleShader();
    void useBlurShader();
    void useCombineShader();
    void useExtractBrightShader();
    ShaderProgram* getBlurShader() { return blurShader; }
    ShaderProgram* getCombineShader() { return combineShader; }
    ShaderProgram* getSunShader() { return sunShader; }
    ShaderProgram* getShadowDepthShader() { return shadowDepthShader; }
    ShaderProgram* getInstancedShadowDepthShader() { return instancedShadowDepthShader; }
    ShaderProgram* getInstancedModelShader() { return instancedModelShader; }
    ShaderProgram* getImposterModelShader() { return imposterModelShader; }
    ShaderProgram* getIrradianceShader() { return irradianceShader; }
    ComputeShader* getNormalMapShader() { return normalMapShader; }
    void init();
    void render(double currentTime);
    void setAspectValues(float frameBufferWidth, float frameBufferHeight);
    void setOrthographicProjection();
    void setPerspectiveProjection();
    void setDummyInverseTranspose();
    void updateProjectionMatrices(int screenWidth, int screenHeight);
    glm::mat4 getShaderViewMatrix();
    GLuint getModelShaderID();
    GLuint getAnimatedModelShaderID();
    GLuint getLightShaderID();
    GLuint getTerrainShaderID();
    GLuint getSkyShaderID();
    GLuint getParticleShaderID();
    GLuint getSunShaderID();
    GLuint getInstancedModelShaderID();
    GLuint getImposterModelShaderID();
    ShaderProgram* getShaderProgramFromID(GLuint id);
    GLuint getShaderIdFromShaderType(ShaderType shaderType);
    ShaderProgram* getShaderProgramFromType(ShaderType shaderType);

    ShaderProgram* modelShader;
    ShaderProgram* animatedModelShader;
    ShaderProgram* lightShader;
    ShaderProgram* terrainShader;
    ShaderProgram* skyShader;
    ShaderProgram* particleShader;
    ShaderProgram* blurShader;
    ShaderProgram* combineShader;
    ShaderProgram* sunShader;
    ShaderProgram* shadowDepthShader;
    ShaderProgram* instancedShadowDepthShader;
    ShaderProgram* instancedModelShader;
    ShaderProgram* imposterModelShader;
    ShaderProgram* irradianceShader;
    ComputeShader* normalMapShader;
    glm::mat4 orthographicMatrix;
};