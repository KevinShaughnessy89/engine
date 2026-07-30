#pragma once

struct CascadeData {
    glm::mat4 lightSpaceMatrices[4];  // Be sure to sync with ShadowState::shadowCascadeCount
    // std140 pads scalar/float arrays to a 16-byte stride per element, so a plain vec4 (rather
    // than float[N]) is what actually matches the shader's uniform block layout -- it also holds
    // up to 4 splits already, so this doesn't need to change again until shadowCascadeCount > 4.
    glm::vec4 cascadeSplits = glm::vec4(0.0f);
    // Per-cascade depth-compare bias in the shadow map's [0,1] depth units (~2 texels of
    // world-space slop / cascade depth span) -- see computeLightSpaceMatrices(). Same vec4-not-
    // float[N] std140 reasoning as cascadeSplits above.
    glm::vec4 cascadeBiases = glm::vec4(0.0f);
};

namespace ShadowState {
inline CascadeData cascadeData;
inline GLuint shadowMapFBO;
inline int shadowMapSize = 2048;
inline float shadowNearPlane = 1.0f;
inline float shadowFarPlane = 600.0f;
// Mutable at runtime via the debug panel's Shadow tab (Renderer::setCascadeCount), but capped
// at 4 by CascadeData::lightSpaceMatrices/cascadeSplits' fixed size -- raising the cap requires
// resizing those (and the matching terrain.frag uniform block) too.
inline int shadowCascadeCount = 4;
inline GLuint shadowTextureArray;
inline GLuint cascadeUBO;
// Fixed unit well past any per-entity material sampler count, so it never collides with
// TextureState::firstFreeTextureID's per-entity counter. Global texture units (shadow map,
// irradiance map, SSAO blur, ...) start at 20 by convention to leave headroom as per-entity
// material texture counts grow -- see SceneState::irradianceMapTextureUnit and
// OcclusionMapGenerator::ssaoBlurTextureUnit for the rest of the block.
inline constexpr int shadowMapTextureUnit = 20;
}  // namespace ShadowState
