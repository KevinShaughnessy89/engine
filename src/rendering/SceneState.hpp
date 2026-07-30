#pragma once

namespace SceneState {
inline GLuint irradianceMap = 0;
inline GLuint environmentMap = 0;
// Same reserved-global-unit convention as ShadowState::shadowMapTextureUnit (20): fixed unit well
// past any per-entity material sampler count, so it never collides with
// TextureState::firstFreeTextureID's per-entity counter.
inline constexpr int irradianceMapTextureUnit = 21;
}  // namespace SceneState