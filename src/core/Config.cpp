#include "core/PrecompiledHeader.hpp"
#include "Config.hpp"

const std::string Config::modelVertexPath = "src/rendering/shaders/model.vert";
const std::string Config::modelFragmentPath = "src/rendering/shaders/model.frag";
const std::string Config::animatedModelVertexPath = "src/rendering/shaders/animatedModel.vert";
const std::string Config::lightVertexPath = "src/rendering/shaders/light.vert";
const std::string Config::lightFragmentPath = "src/rendering/shaders/light.frag";
const std::string Config::terrainVertexPath = "src/rendering/shaders/terrain.vert";
const std::string Config::terrainFragmentPath = "src/rendering/shaders/terrain.frag";
const std::string Config::terrainTessControlPath = "src/rendering/shaders/terrain.tesc";
const std::string Config::terrainTessEvaluationPath = "src/rendering/shaders/terrain.tese";
const std::string Config::skyVertexPath = "src/rendering/shaders/sky.vert";
const std::string Config::skyFragmentPath = "src/rendering/shaders/sky.frag";
const std::string Config::particleVertexPath = "src/rendering/shaders/particle.vert";
const std::string Config::particleFragmentPath = "src/rendering/shaders/particle.frag";
const std::string Config::blurVertexPath = "src/rendering/shaders/blur.vert";
const std::string Config::blurFragmentPath = "src/rendering/shaders/blur.frag";
const std::string Config::combineVertexPath = "src/rendering/shaders/blur.vert";
const std::string Config::combineFragmentPath = "src/rendering/shaders/combine.frag";
const std::string Config::sunVertexPath = "src/rendering/shaders/sun.vert";
const std::string Config::sunFragmentPath = "src/rendering/shaders/sun.frag";
const std::string Config::computeShaderPath = "src/rendering/shaders/compute.comp";
const std::string Config::shadowDepthVertexPath = "src/rendering/shaders/shadowDepth.vert";
const std::string Config::shadowDepthFragmentPath = "src/rendering/shaders/shadowDepth.frag";
const std::string Config::instancedShadowDepthVertexPath =
    "src/rendering/shaders/instancedShadowDepth.vert";
const std::string Config::instancedModelVertexPath =
    "src/rendering/shaders/instancedModel.vert";
const std::string Config::instancedModelFragmentPath =
    "src/rendering/shaders/instancedModel.frag";
const std::string Config::irradianceVertexPath = "src/rendering/shaders/sky.vert";
const std::string Config::irradianceFragmentPath = "src/rendering/shaders/convolution.frag";
const std::string Config::imposterBakeFragmentPath = "src/rendering/shaders/imposterBake.frag";
const std::string Config::imposterModelVertexPath =
    "src/rendering/shaders/imposterModel.vert";
const std::string Config::imposterModelFragmentPath =
    "src/rendering/shaders/imposterModel.frag";
const std::string Config::gBufferVertexPath = "src/rendering/shaders/gbuffer.vert";
const std::string Config::gBufferFragmentPath = "src/rendering/shaders/gbuffer.frag";
const std::string Config::gBufferInstancedVertexPath =
    "src/rendering/shaders/instancedGBuffer.vert";
const std::string Config::gBufferTerrainTessEvaluationPath =
    "src/rendering/shaders/gbufferTerrain.tese";
const std::string Config::gBufferImposterVertexPath = "src/rendering/shaders/imposterGBuffer.vert";
const std::string Config::gBufferAlphaTestFragmentPath =
    "src/rendering/shaders/gbufferAlphaTest.frag";
const std::string Config::gBufferImposterAlphaTestFragmentPath =
    "src/rendering/shaders/gbufferImposterAlphaTest.frag";
const std::string Config::ssaoVertexPath = "src/rendering/shaders/ssaoPassThrough.vert";
const std::string Config::ssaoFragmentPath = "src/rendering/shaders/ssao.frag";
const std::string Config::ssaoBlurVertexPath = "src/rendering/shaders/ssaoPassThrough.vert";
const std::string Config::ssaoBlurFragmentPath = "src/rendering/shaders/ssaoBlur.frag";

const std::string Config::ObjectPath = "assets/objects.json";
const std::string Config::ProjectilePath = "assets/projectiles.json";
const std::string Config::LightPath = "assets/lights.json";
const std::string Config::CameraPath = "assets/camera.json";
const std::string Config::EnvironmentPath = "assets/environment.json";
const std::string Config::ProjectRootDir = "C:/dev/Projects/engine/";
const std::string Config::EnemyPath = "assets/enemies.json";
const std::string Config::ModelsPath = "assets/models.json";

const int Config::SCREEN_WIDTH = 640;
const int Config::SCREEN_HEIGHT = 480;
const int Config::SCREEN_FPS = 60;
const float Config::GRAVITY = -9.81f;
const float Config::EPSILON = 1 / 1e4;
const float Config::SLEEP_THRESHOLD = 0.1f;
const float Config::FORCE_EPSILON = 1.1f;
const float Config::TORQUE_EPSILON = 1.0f;
const float Config::VELOCITY_EPSILON = 1.5f;
const float Config::ANGULAR_VELOCITY_EPSILON = 0.1f;
const float Config::VELOCITY_CHANGE_EPSILON = 0.2f;
const float Config::VELOCITY_CHANGE_EPSILON_SQUARED = 0.08f;
const float Config::ANGULAR_VELOCITY_CHANGE_EPSILON = 0.1f;
