#include "OcclusionMapGenerator.hpp"

#include <random>

#include "DisplayState.hpp"
#include "components/rendering/ChunkRenderable.hpp"
#include "components/rendering/Renderable.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entity/entity.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "rendering/ChunkModel.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/GLState.hpp"
#include "rendering/InstancedModel.hpp"
#include "rendering/Model.hpp"
#include "rendering/ShaderProgram.hpp"
#include "rendering/Texture.hpp"
#include "rendering/UniformUpdater.hpp"

void OcclusionMapGenerator::init() {
    // --- G-buffer: position + normal prepass ---
    glGenFramebuffers(1, &gBufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);

    glGenTextures(1, &gPositionTexture);
    glBindTexture(GL_TEXTURE_2D, gPositionTexture);
    glObjectLabel(GL_TEXTURE, gPositionTexture, -1, "gPositionTexture");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, DisplayState::screenWidth,
                 DisplayState::screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPositionTexture,
                           0);

    glGenTextures(1, &gNormalTexture);
    glBindTexture(GL_TEXTURE_2D, gNormalTexture);
    glObjectLabel(GL_TEXTURE, gNormalTexture, -1, "gNormalTexture");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, DisplayState::screenWidth,
                 DisplayState::screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormalTexture, 0);

    GLuint attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    glGenRenderbuffers(1, &gBufferDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, gBufferDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, DisplayState::screenWidth,
                          DisplayState::screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              gBufferDepthRBO);

    // --- Raw (pre-blur) SSAO output ---
    glGenFramebuffers(1, &ssaoRawFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoRawFBO);

    glGenTextures(1, &ssaoRawTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoRawTexture);
    glObjectLabel(GL_TEXTURE, ssaoRawTexture, -1, "ssaoRawTexture");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, DisplayState::screenWidth, DisplayState::screenHeight, 0,
                 GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoRawTexture, 0);

    // --- Blurred (final) SSAO output --- THIS WAS MISSING
    glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);

    glGenTextures(1, &ssaoBlurTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture);
    glObjectLabel(GL_TEXTURE, ssaoBlurTexture, -1, "ssaoBlurTexture");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, DisplayState::screenWidth, DisplayState::screenHeight, 0,
                 GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    generateKernel();

    gShader = new ShaderProgram(Config::gBufferVertexPath, Config::gBufferFragmentPath);
    // Chunks are drawn with GL_PATCHES (see ChunkModel::render), which requires a program with
    // tessellation control + evaluation stages -- gShader has neither, so terrain needs its own
    // tessellating variant here rather than sharing gShader with the plain-triangle Renderable
    // pass.
    gBufferTerrainShader =
        new ShaderProgram(Config::terrainVertexPath, Config::gBufferFragmentPath,
                          Config::terrainTessControlPath, Config::gBufferTerrainTessEvaluationPath);
    // gBufferAlphaTestFragmentPath: alpha-cutout foliage needs the same discard the forward pass
    // uses, or the G-buffer treats each card's full rectangle as solid and SSAO self-occludes
    // against the transparent parts of overlapping/intersecting cards.
    gBufferInstancedShader =
        new ShaderProgram(Config::gBufferInstancedVertexPath, Config::gBufferAlphaTestFragmentPath);
    // Tree imposters billboard toward the camera (see imposterModel.vert) instead of using
    // the generic per-instance rotation attribute, so they need their own G-buffer transform too.
    gBufferImposterShader = new ShaderProgram(Config::gBufferImposterVertexPath,
                                              Config::gBufferImposterAlphaTestFragmentPath);
    ssaoShader = new ShaderProgram(Config::ssaoVertexPath, Config::ssaoFragmentPath);
    blurShader = new ShaderProgram(Config::ssaoBlurVertexPath, Config::ssaoBlurFragmentPath);
}

void OcclusionMapGenerator::updateFramebufferSize() {
    glBindTexture(GL_TEXTURE_2D, gPositionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, DisplayState::screenWidth,
                 DisplayState::screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

    glBindTexture(GL_TEXTURE_2D, gNormalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, DisplayState::screenWidth,
                 DisplayState::screenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

    glBindRenderbuffer(GL_RENDERBUFFER, gBufferDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, DisplayState::screenWidth,
                          DisplayState::screenHeight);

    glBindTexture(GL_TEXTURE_2D, ssaoRawTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, DisplayState::screenWidth, DisplayState::screenHeight, 0,
                 GL_RED, GL_FLOAT, nullptr);

    glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, DisplayState::screenWidth, DisplayState::screenHeight, 0,
                 GL_RED, GL_FLOAT, nullptr);
}

void OcclusionMapGenerator::generateKernel() {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;  // has no seed, will produce identical results each time

    for (unsigned int i = 0; i < 32; i++) {
        glm::vec3 sample(randomFloats(generator) * 2.0f - 1.0f,
                         randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        // ease in curve, use for LOD
        float scale = float(i) / 32.0f;
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;

        sampleKernel.push_back(sample);
    }

    std::vector<glm::vec3> sampleNoise;
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(randomFloats(generator) * 2.0f - 1.0f,
                        randomFloats(generator) * 2.0f - 1.0f, 0.0f);
        sampleNoise.push_back(noise);
    }

    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glObjectLabel(GL_TEXTURE, noiseTexture, -1, "ssaoNoiseTexture");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &sampleNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void OcclusionMapGenerator::generateOcclusionMap() {
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
    glViewport(0, 0, DisplayState::screenWidth, DisplayState::screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gBufferTerrainShader->use();

    auto chunkView = Registry.view<ChunkRenderable>();
    for (auto entity : chunkView) {
        auto& renderable = chunkView.get<ChunkRenderable>(entity);
        GLState::applyState(renderable.layer, renderable.glStateFlags);
        UniformUpdater::updateUniforms(gBufferTerrainShader, entity);
        // Bind only what gbufferTerrain.tese actually samples -- the full chunkModel.textures set
        // (textureNormal/textureDiffuse/etc.) belongs to the main terrain.frag and doesn't
        // exist in this program, so binding all of it via render(..., bindTextures=true) would
        // just cache a pile of bogus -1 uniform locations and spam UniformUpdater's warning log.
        // normalMapBuffer itself is bound unconditionally inside ChunkModel::render(), so it
        // doesn't need special-casing here.
        for (Texture* texture : renderable.model->textures) {
            if (texture->uniform == "textureDisplacement") {
                texture->bindTexture(gBufferTerrainShader, 0);
            }
        }
        renderable.model->render(gBufferTerrainShader, 0, /*ignoreCulling=*/true,
                                 /*bindTextures=*/false);
    }

    gShader->use();

    auto renderableView = Registry.view<Renderable>();
    for (auto entity : renderableView) {
        auto& renderable = renderableView.get<Renderable>(entity);
        if (renderable.layer == RenderLayer::Skybox || renderable.layer == RenderLayer::Sun)
            continue;
        GLState::applyState(renderable.layer, renderable.glStateFlags);
        UniformUpdater::updateUniforms(gShader, entity);
        renderable.model->render(gShader, 0, /*bindTextures=*/false);
    }

    auto instancedView = Registry.view<InstancedRenderable>();
    for (auto entity : instancedView) {
        auto& renderable = instancedView.get<InstancedRenderable>(entity);
        if (renderable.layer == RenderLayer::Weather) continue;

        ShaderProgram* shader = renderable.layer == RenderLayer::TreeImposters
                                    ? gBufferImposterShader
                                    : gBufferInstancedShader;
        shader->use();
        GLState::applyState(renderable.layer, renderable.glStateFlags);
        UniformUpdater::updateUniforms(shader, entity);

        for (auto& mesh : renderable.model->meshes) {
            // Bind only the one sampler gbufferAlphaTest.frag/gbufferImposterAlphaTest.frag
            // declares (textureDiffuse/treeAtlasTexture) -- binding the mesh's full texture set
            // (e.g. textureNormal too) would cache bogus -1 locations for names these programs
            // don't have, same as the terrain chunk loop above.
            for (Texture* texture : mesh.textures) {
                if (texture->uniform == "textureDiffuse" ||
                    texture->uniform == "treeAtlasTexture") {
                    texture->bindTexture(shader, 0);
                }
            }
            mesh.render(shader, 0, /*bindTextures=*/false);
        }
    }

    // Reset every field to the GLStateFlags baseline rather than nudging off of GLState::current,
    // since this pass follows arbitrary prior geometry passes.
    GLState::setDepthFunc(GL_LESS);
    GLState::setDepthTest(false);
    GLState::setDepthMask(true);
    GLState::setCullFace(true);
    GLState::setDepthClamp(false);
    // ssaoBlur.frag writes a single-component float (no alpha), unlike ssao.frag's vec4. With
    // blend still enabled (the GLStateFlags default) and GL_SRC_ALPHA/GL_ONE_MINUS_SRC_ALPHA still
    // set from the geometry passes above, a shader output with no alpha component is exactly the
    // edge case blending isn't meant to run through -- these are one-shot full-coverage writes to
    // an empty target, not blended composites, so force it off rather than rely on whatever the
    // driver does with a missing alpha.
    GLState::setBlend(false);
    GLState::setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLState::setWireframe(false);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoRawFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    ssaoShader->use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPositionTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormalTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);

    UniformUpdater::updateUniforms(ssaoShader, entt::null);

    ssaoShader->setInt("gPosition", 0);
    ssaoShader->setInt("gNormal", 1);
    ssaoShader->setInt("noiseTexture", 2);

    for (int i = 0; i < sampleKernel.size(); i++) {
        ssaoShader->setVec3("samples[" + std::to_string(i) + "]", sampleKernel[i]);
    }

    Display.getRenderer().drawQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    blurShader->use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoRawTexture);
    blurShader->setInt("ssaoInput", 0);

    Display.getRenderer().drawQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}