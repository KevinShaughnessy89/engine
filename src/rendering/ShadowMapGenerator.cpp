#include "ShadowMapGenerator.hpp"

#include "CameraConstants.hpp"
#include "CameraController.hpp"
#include "components/rendering/ViewState.hpp"
#include "components/rendering/ChunkRenderable.hpp"
#include "components/rendering/InstancedRenderable.hpp"
#include "components/rendering/Renderable.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "rendering/ChunkModel.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/DisplayState.hpp"
#include "rendering/GLState.hpp"
#include "rendering/InstancedModel.hpp"
#include "rendering/Model.hpp"
#include "rendering/ModelLoader.hpp"
#include "rendering/Renderer.hpp"
#include "rendering/ShaderProgram.hpp"
#include "rendering/ShadowState.hpp"
#include "rendering/UniformUpdater.hpp"
#include "rendering/sky/SunHelper.hpp"

namespace {

std::vector<float> computeCascadeSplits(int shadowCascadeCount, float nearPlane, float farPlane,
                                        float lambda) {
    std::vector<float> splits(shadowCascadeCount + 1);

    splits[0] = nearPlane;
    for (int i = 0; i <= shadowCascadeCount; i++) {
        float p = (float)i / shadowCascadeCount;
        float logSplit = nearPlane * std::pow(farPlane / nearPlane, p);
        float uniSplit = nearPlane + (farPlane - nearPlane) * p;
        splits[i] = lambda * logSplit + (1 - lambda) * uniSplit;
    }

    return splits;
}

}  // namespace

namespace ShadowMapGenerator {

void init() {
    initializeShadowMap();
}

void initializeShadowMap() {
    glGenFramebuffers(1, &ShadowState::shadowMapFBO);

    glGenTextures(1, &ShadowState::shadowTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, ShadowState::shadowTextureArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, ShadowState::shadowMapSize,
                 ShadowState::shadowMapSize, ShadowState::shadowCascadeCount, 0, GL_DEPTH_COMPONENT,
                 GL_FLOAT, nullptr);

    // NEAREST + manual PCF below (rather than GL_LINEAR) -- the shadow shader already averages a
    // 3x3 neighborhood of raw depth compares itself, so linear-filtering the depth values first
    // would double up the blending.
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // Outside the shadow map's covered area reads as max depth, i.e. "not in shadow", rather
    // than wrapping/repeating whatever happens to be at the opposite edge.
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    // Sanity-check completeness against cascade layer 0 -- renderShadowPass() re-targets this
    // same attachment to each cascade's layer via glFramebufferTextureLayer every frame.
    glBindFramebuffer(GL_FRAMEBUFFER, ShadowState::shadowMapFBO);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ShadowState::shadowTextureArray,
                              0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Shadow map framebuffer incomplete: " << std::hex << status << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenBuffers(1, &ShadowState::cascadeUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, ShadowState::cascadeUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ShadowState::cascadeData), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // bind this buffer to binding point 0 (arbitrary, but must match shader)
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ShadowState::cascadeUBO);
}

void setCascadeCount(int count) {
    count = std::clamp(count, 1, 4);
    if (count == ShadowState::shadowCascadeCount) return;

    ShadowState::shadowCascadeCount = count;

    glDeleteFramebuffers(1, &ShadowState::shadowMapFBO);
    glDeleteTextures(1, &ShadowState::shadowTextureArray);
    glDeleteBuffers(1, &ShadowState::cascadeUBO);

    initializeShadowMap();
}

void computeLightSpaceMatrices() {
    // Recomputed from scratch every frame -- without clearing first these grow unbounded and
    // renderShadowPass()/the UBO copy (which only ever read index [0, shadowCascadeCount)) would
    // keep reading back frame 1's stale matrices forever.
    lightSpaceMatrices.clear();
    lightProjections.clear();

    ViewState& view = Registry.get<ViewState>(CameraController::activeCamera);
    auto splits = computeCascadeSplits(ShadowState::shadowCascadeCount, DisplayState::nearPlane,
                                       ShadowState::shadowFarPlane, 0.3f);

    for (int i = 0; i < ShadowState::shadowCascadeCount; i++) {
        glm::vec3 cameraPosition = view.position;

        // Fixed radius (this cascade's far split distance), independent of camera
        // orientation/FOV/aspect -- reconstructing it by inverting cascadeProj*viewMatrix
        // (the camera's own, poorly-conditioned near/far-ratio projection) reintroduced the
        // exact numerical instability the original single-shadow-map version deliberately
        // avoided, and made the shadow coverage jitter/resize as the camera rotated instead of
        // staying a stable box around the camera.
        float radius = splits[i + 1];

        float shadowOrthoHalfExtent = radius;

        float worldUnitsPerTexel = (2 * shadowOrthoHalfExtent) / ShadowState::shadowMapSize;

        SunState& sunState = Registry.get<SunState>(ModelRegistry.getEntity("sun"));
        glm::vec3 lightDir = glm::normalize(sunState.position);

        glm::vec3 sceneCenter = cameraPosition;  // center the shadow map on the camera's current
                                                 // position, not the frustum's

        float lightDistance = radius + 10.0f;

        glm::vec3 eye = lightDir * lightDistance;

        // use a fixed-space reference matrix for lightView
        glm::mat4 lightView = glm::lookAt(eye, glm::vec3(0.f), CameraConstants::WORLD_UP_AXIS);

        glm::vec4 lightSpaceSceneCenter = lightView * glm::vec4(sceneCenter, 1.f);

        lightSpaceSceneCenter.x =
            std::floor(lightSpaceSceneCenter.x / worldUnitsPerTexel) * worldUnitsPerTexel;
        lightSpaceSceneCenter.y =
            std::floor(lightSpaceSceneCenter.y / worldUnitsPerTexel) * worldUnitsPerTexel;

        glm::vec4 snappedToGridSceneCenter = glm::inverse(lightView) * lightSpaceSceneCenter;

        glm::mat4 lightViewSnapped =
            glm::lookAt(glm::vec3(snappedToGridSceneCenter) + lightDir * lightDistance,
                        glm::vec3(snappedToGridSceneCenter), CameraConstants::WORLD_UP_AXIS);

        // eye sits `lightDistance` from center, and every corner is within `radius` of center, so
        // the closest/farthest a corner can be from eye is lightDistance -/+ radius -- symmetric
        // regardless of camera facing direction.
        float shadowNearPlaneDynamic = std::max(0.1f, lightDistance - radius);
        float shadowFarPlaneDynamic = lightDistance + radius + 10.0f;  // margin for safety

        glm::mat4 lightProjection =
            glm::ortho(-shadowOrthoHalfExtent, shadowOrthoHalfExtent, -shadowOrthoHalfExtent,
                       shadowOrthoHalfExtent, shadowNearPlaneDynamic, shadowFarPlaneDynamic);

        lightSpaceMatrices.push_back(lightProjection * lightViewSnapped);
        lightProjections.push_back(lightProjection);

        // Depth-compare bias for this cascade, expressed in the [0,1] depth range the shadow map
        // stores: ~2 texels' worth of world-space slop, normalized by the cascade's depth span.
        // A flat NDC constant (the old 0.005..0.05) is worth *meters* over these ranges and
        // erased all terrain self-shadowing.
        ShadowState::cascadeData.cascadeBiases[i] =
            (2.0f * worldUnitsPerTexel) / (shadowFarPlaneDynamic - shadowNearPlaneDynamic);
    }

    for (int i = 0; i < ShadowState::shadowCascadeCount; i++) {
        ShadowState::cascadeData.lightSpaceMatrices[i] = lightSpaceMatrices[i];
        ShadowState::cascadeData.cascadeSplits[i] = splits[i + 1];
    }
    // The shader's cascade-selection loop always checks all 4 UBO slots; when running with fewer
    // cascades the unused slots would otherwise keep whatever matrices/splits the previous count
    // last wrote (frozen at an old camera position -> everything projects to the border -> no
    // shadows). Park the splits at FLT_MAX so selection can never land past the live cascades,
    // and mirror the last live matrix/bias in case of FP edge cases.
    for (int i = ShadowState::shadowCascadeCount; i < 4; i++) {
        ShadowState::cascadeData.lightSpaceMatrices[i] =
            lightSpaceMatrices[ShadowState::shadowCascadeCount - 1];
        ShadowState::cascadeData.cascadeSplits[i] = std::numeric_limits<float>::max();
        ShadowState::cascadeData.cascadeBiases[i] =
            ShadowState::cascadeData.cascadeBiases[ShadowState::shadowCascadeCount - 1];
    }
}

void renderShadowPass() {
    computeLightSpaceMatrices();

    ShaderProgram* shadowShader = Display.getShadowDepthShader();

    glViewport(0, 0, ShadowState::shadowMapSize, ShadowState::shadowMapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, ShadowState::shadowMapFBO);
    // The previous frame's skybox leaves depthMask=false, and both glClear and the depth-only
    // draws below silently no-op while the write mask is off -- without this the cascade
    // textures freeze at their frame-1 contents. combine() (last thing each frame) also leaves
    // GL_DEPTH_TEST disabled for its fullscreen quad, and depth writes only happen while depth
    // testing is enabled -- with it off, this whole pass rasterizes but stores nothing and the
    // maps stay at their cleared 1.0. Routed through the GLState setters so GLState::current stays
    // authoritative rather than being hand-updated alongside each raw gl call.
    GLState::setDepthMask(true);
    GLState::setDepthTest(true);
    GLState::setCullFace(false);
    for (int i = 0; i < ShadowState::shadowCascadeCount; i++) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  ShadowState::shadowTextureArray, 0, i);
        // Clear per layer: glClear only touches the currently attached layer, so a single clear
        // before the loop would leave every other cascade accumulating stale min-depths forever.
        glClear(GL_DEPTH_BUFFER_BIT);

        shadowShader->use();
        shadowShader->setMat4("lightSpaceMatrix", lightSpaceMatrices[i]);

        auto chunkView = Registry.view<ChunkRenderable>();
        for (auto entity : chunkView) {
            auto& renderable = chunkView.get<ChunkRenderable>(entity);
            shadowShader->setMat4("model", glm::mat4(1.0f));
            renderable.model->render(shadowShader, 0, /*ignoreCulling=*/true,
                                     /*bindTextures=*/false);
        }

        auto renderableView = Registry.view<Renderable>();
        for (auto entity : renderableView) {
            auto& renderable = renderableView.get<Renderable>(entity);
            if (renderable.layer == RenderLayer::Skybox || renderable.layer == RenderLayer::Sun)
                continue;

            shadowShader->setMat4("model", UniformUpdater::calculateModelMatrix(entity));
            renderable.model->render(shadowShader, 0, /*bindTextures=*/false);
        }

        ShaderProgram* instancedShadowShader = Display.getInstancedShadowDepthShader();
        instancedShadowShader->use();
        instancedShadowShader->setMat4("lightSpaceMatrix", lightSpaceMatrices[i]);

        auto instancedView = Registry.view<InstancedRenderable>();
        for (auto entity : instancedView) {
            auto& renderable = instancedView.get<InstancedRenderable>(entity);
            if (renderable.layer == RenderLayer::Weather) continue;

            for (auto& mesh : renderable.model->meshes) {
                mesh.render(instancedShadowShader, 0, /*bindTextures=*/false);
            }
        }
    }

    GLState::setCullFace(true);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}  // namespace ShadowMapGenerator
