#pragma once

class ShaderProgram;

namespace OcclusionMapGenerator {
inline GLuint ssaoRawFBO;
inline GLuint ssaoRawTexture;
inline GLuint gBufferFBO;
inline GLuint gPositionTexture;
inline GLuint gNormalTexture;
inline GLuint gBufferDepthRBO;
inline GLuint ssaoBlurFBO;
inline GLuint ssaoBlurTexture;
inline GLuint noiseTexture;

inline ShaderProgram* gShader;
inline ShaderProgram* gBufferTerrainShader;
inline ShaderProgram* ssaoShader;
inline ShaderProgram* gBufferInstancedShader;
inline ShaderProgram* gBufferImposterShader;
inline ShaderProgram* blurShader;

inline std::vector<glm::vec3> sampleKernel;

// Same reserved-global-unit convention as ShadowState::shadowMapTextureUnit (20) and
// SceneState::irradianceMapTextureUnit (21).
inline constexpr int ssaoBlurTextureUnit = 22;

void init();
void generateOcclusionMap();
void generateKernel();
// Reallocates the G-buffer + SSAO textures/renderbuffer at DisplayState::screenWidth/screenHeight
// -- call after any change to those (e.g. on window resize), mirroring
// Renderer::updateFramebufferWindowSize() for the main scene framebuffer.
void updateFramebufferSize();
}  // namespace OcclusionMapGenerator