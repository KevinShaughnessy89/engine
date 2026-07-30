#pragma once
#include "core/PrecompiledHeader.hpp"

#include <string>

namespace Config {
// Use JSON to set up configuration data (implement soon)
extern const std::string modelVertexPath;
extern const std::string modelFragmentPath;
extern const std::string animatedModelVertexPath;
extern const std::string lightVertexPath;
extern const std::string lightFragmentPath;
extern const std::string terrainVertexPath;
extern const std::string terrainFragmentPath;
extern const std::string terrainTessControlPath;
extern const std::string terrainTessEvaluationPath;
extern const std::string skyVertexPath;
extern const std::string skyFragmentPath;
extern const std::string particleVertexPath;
extern const std::string particleFragmentPath;
extern const std::string blurVertexPath;
extern const std::string blurFragmentPath;
extern const std::string combineVertexPath;
extern const std::string combineFragmentPath;
extern const std::string sunFragmentPath;
extern const std::string sunVertexPath;
extern const std::string computeShaderPath;
extern const std::string shadowDepthVertexPath;
extern const std::string shadowDepthFragmentPath;
extern const std::string instancedShadowDepthVertexPath;
extern const std::string instancedModelVertexPath;
extern const std::string instancedModelFragmentPath;
extern const std::string irradianceVertexPath;
extern const std::string irradianceFragmentPath;
extern const std::string imposterBakeFragmentPath;
extern const std::string imposterModelVertexPath;
extern const std::string imposterModelFragmentPath;
extern const std::string gBufferVertexPath;
extern const std::string gBufferFragmentPath;
extern const std::string gBufferInstancedVertexPath;
extern const std::string gBufferTerrainTessEvaluationPath;
extern const std::string gBufferImposterVertexPath;
extern const std::string gBufferAlphaTestFragmentPath;
extern const std::string gBufferImposterAlphaTestFragmentPath;
extern const std::string ssaoVertexPath;
extern const std::string ssaoFragmentPath;
extern const std::string ssaoBlurVertexPath;
extern const std::string ssaoBlurFragmentPath;

extern const std::string ObjectPath;
extern const std::string ProjectilePath;
extern const std::string LightPath;
extern const std::string CameraPath;
extern const std::string EnvironmentPath;
extern const std::string EnemyPath;
extern const std::string ModelsPath;

extern const std::string ProjectRootDir;
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const int SCREEN_FPS;
extern const float GRAVITY;
extern const float EPSILON;
extern const float SLEEP_THRESHOLD;
extern const float FORCE_EPSILON;
extern const float TORQUE_EPSILON;
extern const float VELOCITY_EPSILON;
extern const float ANGULAR_VELOCITY_EPSILON;
extern const float VELOCITY_CHANGE_EPSILON;
extern const float VELOCITY_CHANGE_EPSILON_SQUARED;
extern const float ANGULAR_VELOCITY_CHANGE_EPSILON;
}  // namespace Config
