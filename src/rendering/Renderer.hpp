#pragma once
#include "GLState.hpp"
#include "ImageBasedLighting.hpp"
#include "InstancedMeshVertex.hpp"
#include "Model.hpp"
#include "RenderLayer.hpp"
#include "ShaderType.hpp"
#include "ShadowMapGenerator.hpp"
#include "UniformData.hpp"
#include "UniformUpdater.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"

struct InstancedRenderable;

struct ChunkVertexCache {
    std::vector<glm::vec3> vertices;
    int numChunks = 0;
};

struct RenderLayerConfig {
    RenderLayer layer;
    ShaderType shaderType;
};

// Axis-aligned X/Z bounds of the view frustum's world-space corners, used as a cheap coarse
// reject before the precise per-plane frustum test.
struct FrustumXZBounds {
    float minX, maxX, minZ, maxZ;
};

class Renderer {
   public:
    Renderer() {}

    ImageBasedLighting imageBasedLighting;

    std::vector<RenderLayerConfig> renderOrder = {
        {RenderLayer::Sun, ShaderType::Sun},
        {RenderLayer::Terrain, ShaderType::Terrain},
        {RenderLayer::Objects, ShaderType::Model},
        {RenderLayer::Animated, ShaderType::AnimatedModel},
        {RenderLayer::Skybox, ShaderType::Sky},
        {RenderLayer::TreeImposters, ShaderType::ImposterModel},
        {RenderLayer::Projectiles, ShaderType::Model},
        {RenderLayer::InstancedObjects, ShaderType::InstancedModel},
        {RenderLayer::Weather, ShaderType::Particle},
    };

    void renderBatches(double currentTime);

    ShaderProgram* useShader(ShaderType shaderType);

    void cullInstances(InstancedRenderable& renderable, entt::entity entity,
                       const std::array<glm::vec4, 6>& frustumPlanes,
                       const FrustumXZBounds& frustumXZBounds, const glm::vec3& cameraPosition,
                       std::vector<InstancedMeshVertex>& visibleInstances);

    void init();
    void updateFramebufferWindowSize();
    void initializeFramebuffer();

    void getBatches(std::unordered_map<RenderLayer, std::vector<entt::entity>>& batches,
                    std::unordered_map<RenderLayer, std::vector<entt::entity>>& instancedBatches,
                    bool shouldCull = true);

    // cullInstances() output buffer; a member so it stays warm across frames, not realloc'd each
    // one.
    std::vector<InstancedMeshVertex> visibleInstancesScratch;
    void initializeQuad();
    void drawQuad();
    void renderWithBlur();
    void combine();
    void blur();
    int windowWidth;
    int windowHeight;
    GLuint rbo;
    GLuint quadVAO, quadVBO;
    GLuint sceneFBO, brightFBO, sceneTex, brightTex;
    GLuint boneSSBO;
    GLuint pingpongFBO[2], pingpongTex[2];
    bool horizontal = true;

    void setCascadeCount(int count) { ShadowMapGenerator::setCascadeCount(count); }

    void setWindowHeight(int height) { this->windowHeight = height; };
    void setWindowWidth(int width) { this->windowWidth = width; };

    std::array<glm::vec4, 6> getFrustrumPlanes();
    std::array<glm::vec3, 8> getFrustumCornersWorldSpace();
    FrustumXZBounds getFrustumXZBounds();
    bool shouldFrustrumCull(const std::array<glm::vec4, 6>& frustumPlanes, const glm::vec3& min,
                            const glm::vec3& max);
    bool shouldOccludeCull(const glm::vec3& targetPoint);
    bool aabbOutsidePlane(const glm::vec4& plane, const glm::vec3& min, const glm::vec3& max);
};
