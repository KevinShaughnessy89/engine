#pragma once
#include "core/PrecompiledHeader.hpp"

namespace ShadowMapGenerator {
inline std::vector<glm::mat4> lightSpaceMatrices;
inline std::vector<glm::mat4> lightProjections;

void init();
void renderShadowPass();
void computeLightSpaceMatrices();
// Clamps to [1, 4] (see ShadowState::shadowCascadeCount), then frees and rebuilds the shadow
// depth texture array/FBO/UBO at the new depth. No-op if count doesn't change.
void setCascadeCount(int count);

void initializeShadowMap();
}  // namespace ShadowMapGenerator
