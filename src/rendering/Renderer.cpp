#include "Renderer.hpp"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <tracy/Tracy.hpp>

#include "CameraConstants.hpp"
#include "CameraController.hpp"
#include "TextureState.hpp"
#include "UniformUpdater.hpp"
#include "character/AnimationRenderer.hpp"
#include "components/rendering/ChunkRenderable.hpp"
#include "components/rendering/InstanceLOD.hpp"
#include "components/rendering/InstancedRenderable.hpp"
#include "components/rendering/Renderable.hpp"
#include "components/rendering/ViewState.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "environment/ChunkManager.hpp"
#include "rendering/ChunkModel.hpp"
#include "rendering/DebugState.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/DisplayState.hpp"
#include "rendering/InstancedModel.hpp"
#include "rendering/Model.hpp"
#include "rendering/ModelLoader.hpp"
#include "rendering/OcclusionMapGenerator.hpp"
#include "rendering/ShaderProgram.hpp"

void Renderer::init() {
    initializeFramebuffer();
    initializeQuad();
    initializeAxisLines();
    ShadowMapGenerator::init();
    OcclusionMapGenerator::init();
    imageBasedLighting.init();

    glGenBuffers(1, &boneSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, boneSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 AnimationRenderer::MAX_ANIMATED_ENTITIES *
                     AnimationRenderer::MAX_BONES_PER_SKELETON * sizeof(glm::mat4),
                 nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, boneSSBO);
}

void Renderer::updateFramebufferWindowSize() {
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT,
                 NULL);
    glBindTexture(GL_TEXTURE_2D, brightTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT,
                 NULL);

    for (int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT,
                     nullptr);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, windowWidth, windowHeight);
}

void debug() {
    float vertices[] = {-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f};

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // layout (location = 0) in vec3 aPos;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Use your debug shader
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Optional: clean up immediately
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void printVariant(const std::string& name, const UniformValue& value) {
    std::cout << "Updating " << name << " : ";

    if (std::holds_alternative<int>(value)) {
        std::cout << std::get<int>(value);
    } else if (std::holds_alternative<float>(value)) {
        std::cout << std::get<float>(value);
    } else if (std::holds_alternative<glm::vec2>(value)) {
        glm::vec2 v = std::get<glm::vec2>(value);
        std::cout << v.x << ", " << v.y;
    } else if (std::holds_alternative<glm::vec3>(value)) {
        glm::vec3 v = std::get<glm::vec3>(value);
        std::cout << v.x << ", " << v.y << ", " << v.z;
    } else if (std::holds_alternative<glm::vec4>(value)) {
        glm::vec4 v = std::get<glm::vec4>(value);
        std::cout << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
    } else if (std::holds_alternative<glm::mat3>(value)) {
        glm::mat3 m = std::get<glm::mat3>(value);
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                std::cout << m[col][row];
                if (col < 2) std::cout << ", ";
            }
            if (row < 2) std::cout << " | ";
        }
    } else if (std::holds_alternative<glm::mat4>(value)) {
        glm::mat4 m = std::get<glm::mat4>(value);
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                std::cout << m[col][row];
                if (col < 3) std::cout << ", ";
            }
            if (row < 3) std::cout << " | ";
        }
    } else {
        std::cout << "Unknown type";
    }

    std::cout << std::endl;
}

ShaderProgram* Renderer::useShader(ShaderType shaderType) {
    ShaderProgram* shader = Display.getShaderProgramFromType(shaderType);
    shader->use();
    return shader;
}

void Renderer::cullInstances(InstancedRenderable& renderable, entt::entity entity,
                             const std::array<glm::vec4, 6>& frustumPlanes,
                             const FrustumXZBounds& frustumXZBounds,
                             const glm::vec3& cameraPosition,
                             std::vector<InstancedMeshVertex>& visibleInstances) {
    ZoneScoped;
    ZoneValue(renderable.model->allInstances.size());
    visibleInstances.clear();
    visibleInstances.reserve(renderable.model->allInstances.size());

    glm::vec3 localExtent;
    float localRadius;
    InstanceLOD* lod;
    std::array<glm::vec3, 6> planeNormals;
    {
        ZoneScopedN("cullInstances: setup (bounding sphere + LOD lookup + plane normals)");
        // A bounding sphere centered on the instance's own placement point is rotation-invariant
        // (instancedModel.vert only yaws the mesh around that point), so one radius computed
        // from the model's local-space AABB covers every instance regardless of its rotation.
        glm::vec3 localMin = renderable.model->getMinCoordinates();
        glm::vec3 localMax = renderable.model->getMaxCoordinates();
        localExtent = glm::vec3(std::max(std::abs(localMin.x), std::abs(localMax.x)),
                                std::max(std::abs(localMin.y), std::abs(localMax.y)),
                                std::max(std::abs(localMin.z), std::abs(localMax.z)));
        localRadius = glm::length(localExtent);

        lod = Registry.try_get<InstanceLOD>(entity);

        // frustumPlanes is fixed for the whole call, so truncate to vec3 once here instead of
        // once per plane per instance in the hot loop below.
        for (int i = 0; i < 6; ++i) planeNormals[i] = glm::vec3(frustumPlanes[i]);
    }

    {
        ZoneScopedN("cullInstances: per-instance loop (frustum + LOD test)");
        ZoneValue(visibleInstances.capacity());
        for (const auto& instance : renderable.model->allInstances) {
            float worldRadius = localRadius * instance.size.x;

            // Cheap coarse reject against the frustum's world-space X/Z bounds before the precise
            // per-plane test below. This can only ever be looser than the real frustum (it's the
            // AABB of the frustum's corners projected onto X/Z, expanded by the instance's own
            // radius), so it never rejects something the precise test would have kept.
            if (instance.position.x + worldRadius < frustumXZBounds.minX ||
                instance.position.x - worldRadius > frustumXZBounds.maxX ||
                instance.position.z + worldRadius < frustumXZBounds.minZ ||
                instance.position.z - worldRadius > frustumXZBounds.maxZ) {
                continue;
            }

            bool outside = false;
            for (int i = 0; i < 6; ++i) {
                float distance = glm::dot(planeNormals[i], instance.position) + frustumPlanes[i].w;
                if (distance < -worldRadius) {
                    outside = true;
                    break;
                }
            }
            if (outside) continue;

            if (lod) {
                bool isFar = glm::distance2(cameraPosition, instance.position) >=
                             lod->switchDistance * lod->switchDistance;
                if (isFar != lod->isFarVariant) continue;
            }

            visibleInstances.push_back(instance);
        }
        ZoneValue(visibleInstances.size());
    }
}

namespace {
// Model::min/max are in mesh-local space (raw import-time vertex extents), not world space, so
// they must be transformed by the entity's own model matrix before being tested against
// world-space frustum planes -- otherwise anything moved by a KinematicBody/Position (e.g. the
// character walking away from the origin) gets culled using its bounds as if it never moved.
void transformAABBToWorld(const glm::mat4& model, const glm::vec3& localMin,
                          const glm::vec3& localMax, glm::vec3& outMin, glm::vec3& outMax) {
    outMin = glm::vec3(std::numeric_limits<float>::infinity());
    outMax = glm::vec3(-std::numeric_limits<float>::infinity());
    for (int i = 0; i < 8; i++) {
        glm::vec3 corner((i & 1) ? localMax.x : localMin.x, (i & 2) ? localMax.y : localMin.y,
                         (i & 4) ? localMax.z : localMin.z);
        glm::vec3 worldCorner = glm::vec3(model * glm::vec4(corner, 1.0f));
        outMin = glm::min(outMin, worldCorner);
        outMax = glm::max(outMax, worldCorner);
    }
}
}  // namespace

void Renderer::getBatches(
    std::unordered_map<RenderLayer, std::vector<entt::entity>>& batches,
    std::unordered_map<RenderLayer, std::vector<entt::entity>>& instancedBatches, bool shouldCull) {
    ZoneScoped;
    std::array<glm::vec4, 6> frustumPlanes = getFrustrumPlanes();
    FrustumXZBounds frustumXZBounds = getFrustumXZBounds();
    glm::vec3 cameraPosition = Registry.get<ViewState>(CameraController::activeCamera).position;

    auto view1 = Registry.view<Renderable>();
    auto view2 = Registry.view<InstancedRenderable>();
    auto view3 = Registry.view<ChunkRenderable>();

    {
        ZoneScopedN("getBatches: Renderable cull");
        for (auto entity : view1) {
            auto& t = view1.get<Renderable>(entity);
            // Sun/Skybox are positioned via a dynamic transform in UniformUpdater rather than the
            // model's own static geometry
            if (t.layer == RenderLayer::Sun || t.layer == RenderLayer::Skybox) {
                t.shouldRender = true;
            } else if (shouldCull) {
                glm::vec3 worldMin, worldMax;
                transformAABBToWorld(UniformUpdater::calculateModelMatrix(entity),
                                     t.model->getMinCoordinates(), t.model->getMaxCoordinates(),
                                     worldMin, worldMax);
                t.shouldRender = !shouldFrustrumCull(frustumPlanes, worldMin, worldMax);
            } else {
                t.shouldRender = true;
            }
            DebugState::LayerRenderStats& stats = DebugState::layerRenderStats[t.layer];
            stats.submitted++;
            if (t.shouldRender) stats.rendered++;
            batches[t.layer].push_back(entity);
        }
    }

    {
        ZoneScopedN("getBatches: InstancedRenderable cull");
        for (auto instancedEntity : view2) {
            auto& renderData = view2.get<InstancedRenderable>(instancedEntity);
            DebugState::LayerRenderStats& stats = DebugState::layerRenderStats[renderData.layer];

            if (shouldCull && !renderData.model->allInstances.empty()) {
                cullInstances(renderData, instancedEntity, frustumPlanes, frustumXZBounds,
                              cameraPosition, visibleInstancesScratch);
                renderData.model->updateVisibleInstances(visibleInstancesScratch);

            } else {
                renderData.model->updateVisibleInstances(renderData.model->allInstances);
            }
            stats.submitted +=
                shouldCull ? visibleInstancesScratch.size() : renderData.model->allInstances.size();
            stats.rendered += shouldCull ? (int)visibleInstancesScratch.size()
                                         : (int)renderData.model->allInstances.size();
            instancedBatches[renderData.layer].push_back(instancedEntity);
        }
    }

    {
        ZoneScopedN("getBatches: ChunkRenderable cull");
        for (auto chunkEntity : view3) {
            auto& t = view3.get<ChunkRenderable>(chunkEntity);
            DebugState::LayerRenderStats& stats = DebugState::layerRenderStats[t.layer];
            for (auto& slot : t.model->slots) {
                if (!slot.active) continue;
                slot.shouldRender = !shouldFrustrumCull(frustumPlanes, slot.aabbMin, slot.aabbMax);
                stats.submitted++;
                if (slot.shouldRender) stats.rendered++;
            }
            batches[t.layer].push_back(chunkEntity);
        }
    }
}

void Renderer::renderBatches(double deltaTime) {
    ZoneScoped;

    std::unordered_map<RenderLayer, std::vector<entt::entity>> batches;
    std::unordered_map<RenderLayer, std::vector<entt::entity>> instancedBatches;

    DebugState::layerRenderStats.clear();

    // Must run before shadow/occlusion below -- they draw this frame's culled instance buffers.
    getBatches(batches, instancedBatches);

    {
        ZoneScopedN("renderBatches: shadow pass");
        ShadowMapGenerator::renderShadowPass();
    }
    {
        ZoneScopedN("renderBatches: occlusion map");
        OcclusionMapGenerator::generateOcclusionMap();
    }

    GLState::setDepthMask(true);
    GLState::setCullFace(true);
    GLState::setDepthTest(true);

    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_FRAMEBUFFER_SRGB);

    GLuint attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);  // Enable MRT for this draw call

    for (const auto currentBatch : renderOrder) {
        ZoneScopedN("renderBatches: layer");
        const char* layerName = renderLayerToString(currentBatch.layer);
        ZoneText(layerName, strlen(layerName));

        auto& renderableEntities = batches[currentBatch.layer];
        auto& instancedRenderableEntities = instancedBatches[currentBatch.layer];

        ShaderProgram* shader = useShader(currentBatch.shaderType);

        if (currentBatch.layer == RenderLayer::Animated) {
            AnimationRenderer::updateAnimationState(deltaTime, boneSSBO);
        }

        for (auto entity : renderableEntities) {
            auto [renderable, chunkRenderable] =
                Registry.try_get<Renderable, ChunkRenderable>(entity);
            if (renderable && !renderable->shouldRender) continue;

            TextureState::firstFreeTextureID = 0;
            UniformUpdater::updateUniforms(shader, entity);
            UniformUpdater::updateTextureUniform(shader, entity);

            GLState::applyState(currentBatch.layer, renderable ? renderable->glStateFlags
                                                               : chunkRenderable->glStateFlags);

            if (renderable) {
                for (auto& mesh : renderable->model->meshes) {
                    UniformUpdater::updateNormalMapUniform(shader, *mesh);
                    UniformUpdater::updateSpecularMapUniform(shader, *mesh);
                    UniformUpdater::updateEmissiveMapUniform(shader, *mesh);
                    UniformUpdater::updateShininessMapUniform(shader, *mesh);
                    mesh->render(shader, TextureState::firstFreeTextureID);
                }
            } else {
                chunkRenderable->model->render(shader, TextureState::firstFreeTextureID);
            }
        }

        for (auto entity : instancedRenderableEntities) {
            if (!Registry.all_of<InstancedRenderable>(entity)) continue;

            auto& renderData = Registry.get<InstancedRenderable>(entity);
            if (renderData.shouldRender) {
                TextureState::firstFreeTextureID = 0;

                UniformUpdater::updateUniforms(shader, entity);
                UniformUpdater::updateTextureUniform(shader, entity);

                GLState::applyState(currentBatch.layer, renderData.glStateFlags);
                for (auto& mesh : renderData.model->meshes) {
                    UniformUpdater::updateNormalMapUniform(shader, mesh);
                    UniformUpdater::updateAlphaMapUniform(shader, mesh);
                    if (mesh.hasAlphaMap) {
                        GLState::setDepthMask(false);
                        GLState::setCullFace(false);
                    }
                    mesh.render(shader, TextureState::firstFreeTextureID);
                    if (mesh.hasAlphaMap) {
                        GLState::setDepthMask(true);
                        GLState::setCullFace(true);
                    }
                }
            }
        }

        if (currentBatch.layer == RenderLayer::Objects && DebugState::showAxisGizmo) {
            auto& view = Registry.get<ViewState>(CameraController::activeCamera);
            shader->setMat4("view", glm::lookAtRH(view.position, view.position + view.forward,
                                                  CameraConstants::WORLD_UP_AXIS));
            shader->setMat4("projection", DisplayState::perspectiveMatrix);
            drawAxisLines(shader);
        }
    }
    {
        ZoneScopedN("renderBatches: blur");
        renderWithBlur();
    }
    glDisable(GL_FRAMEBUFFER_SRGB);
}

void Renderer::initializeQuad() {
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Positions
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture Coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);  // Unbind
}

void Renderer::drawQuad() {
    glBindVertexArray(quadVAO);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// World-space X/Y/Z gizmo through the origin, reusing modelShader (model.vert's normal/texCoord
// attributes are just zeroed -- harmless, since model.frag no longer reads them) so drawing it
// costs a VAO swap and no shader/program switch.
void Renderer::initializeAxisLines() {
    float axisLength = 500.0f;
    float axisVertices[] = {
        // position (vec4)             normal (vec3)      texCoords (vec2)
        -axisLength, 0.0f,        0.0f,        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        axisLength,  0.0f,        0.0f,        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

        0.0f,        -axisLength, 0.0f,        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f,        axisLength,  0.0f,        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

        0.0f,        0.0f,        -axisLength, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f,        0.0f,        axisLength,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    };

    glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);

    glBindVertexArray(axisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Renderer::drawAxisLines(ShaderProgram* shader) {
    shader->setMat4("model", glm::mat4(1.0f));
    glBindVertexArray(axisVAO);

    shader->setVec3("tint", glm::vec3(1.0f, 0.0f, 0.0f));
    glDrawArrays(GL_LINES, 0, 2);
    shader->setVec3("tint", glm::vec3(0.0f, 1.0f, 0.0f));
    glDrawArrays(GL_LINES, 2, 2);
    shader->setVec3("tint", glm::vec3(0.0f, 0.0f, 1.0f));
    glDrawArrays(GL_LINES, 4, 2);

    glBindVertexArray(0);
}

void Renderer::initializeFramebuffer() {
    // Scene framebuffer - just stores the rendered scene
    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    glGenTextures(1, &sceneTex);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTex, 0);

    glGenTextures(1, &brightTex);
    glBindTexture(GL_TEXTURE_2D, brightTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, brightTex, 0);

    GLuint attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cout << "OpenGL error after texture setup: " << std::hex << err << std::endl;
    }
    // Add depth for scene rendering
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, windowWidth, windowHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    std::string statusString;

    switch (status) {
        case GL_FRAMEBUFFER_COMPLETE:
            statusString = "GL_FRAMEBUFFER_COMPLETE";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            statusString = "GL_FRAMEBUFFER_UNSUPPORTED";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
            break;
        default:
            statusString = "UNKNOWN_STATUS";
            break;
    }

    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongTex);
    for (int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongTex[i]);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT,
                     nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongTex[i],
                               0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderWithBlur() {
    blur();
    combine();
}

void Renderer::blur() {
    ShaderProgram* blurShader = Display.getBlurShader();

    blurShader->use();

    bool firstIteration = true;
    int blurPasses = 10;
    horizontal = false;

    for (int i = 0; i < blurPasses; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        glViewport(0, 0, windowWidth, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT);
        blurShader->setBool("horizontal", horizontal);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, firstIteration ? brightTex : pingpongTex[!horizontal]);
        blurShader->setInt("image", 0);
        drawQuad();  // draws with current shader and texture

        horizontal = !horizontal;
        if (firstIteration) firstIteration = false;
    }
}

void Renderer::combine() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    // Skybox (last thing drawn before this) leaves depthMask=false and depthFunc=GL_LEQUAL on
    // GLState::current. depthMask=false means the depth clear below would silently no-op, and
    // the full-screen quad would then be depth-tested against whatever's left in the default
    // framebuffer's depth buffer -- normally harmless leftover data, but a real window resize
    // (e.g. alt-enter fullscreen) makes the driver reallocate that depth buffer with fresh
    // contents, which can fail GL_LEQUAL and discard the quad entirely, leaving the screen
    // showing just the clear color. The quad doesn't need depth testing at all, so clear
    // properly and disable it outright, via the setters so GLState::current stays in sync.
    GLState::setDepthMask(true);
    GLState::setDepthTest(false);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ShaderProgram* combineShader = Display.getCombineShader();
    combineShader->use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex);  // original scene
    combineShader->setInt("sceneTex", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongTex[!horizontal]);  // final blurred bloom
    combineShader->setInt("bloomBlur", 1);

    drawQuad();
}
bool Renderer::shouldFrustrumCull(const std::array<glm::vec4, 6>& frustumPlanes,
                                  const glm::vec3& min, const glm::vec3& max) {
    for (const auto& plane : frustumPlanes) {
        if (aabbOutsidePlane(plane, min, max)) {
            return true;  // Cull this model
        }
    }
    return false;
}

std::array<glm::vec4, 6> Renderer::getFrustrumPlanes() {
    ViewState& view = Registry.get<ViewState>(CameraController::activeCamera);

    glm::mat4 viewMatrix =
        glm::lookAtRH(view.position, view.position + view.forward, CameraConstants::WORLD_UP_AXIS);

    glm::mat4 vp = DisplayState::perspectiveMatrix * viewMatrix;

    std::array<glm::vec4, 6> planes;

    planes[0] = glm::row(vp, 3) + glm::row(vp, 0);  // Left
    planes[1] = glm::row(vp, 3) - glm::row(vp, 0);  // Right
    planes[2] = glm::row(vp, 3) + glm::row(vp, 1);  // Bottom
    planes[3] = glm::row(vp, 3) - glm::row(vp, 1);  // Top
    planes[4] = glm::row(vp, 3) + glm::row(vp, 2);  // Near
    planes[5] = glm::row(vp, 3) - glm::row(vp, 2);  // Far

    for (auto& p : planes) {
        float len = glm::length(glm::vec3(p));
        p /= len;
    }
    return planes;
}

std::array<glm::vec3, 8> Renderer::getFrustumCornersWorldSpace() {
    ViewState& view = Registry.get<ViewState>(CameraController::activeCamera);

    glm::mat4 viewMatrix =
        glm::lookAtRH(view.position, view.position + view.forward, CameraConstants::WORLD_UP_AXIS);

    // Build corners from the projection's near/far tangent extents and rotate them into world
    // space via inverse(view) alone, rather than inverting the full view-projection matrix.
    // With nearPlane=0.1/farPlane=1000 the projection matrix is poorly conditioned (10000:1
    // depth ratio), so inverting view-projection every frame recovered corners that jittered by
    // a fraction of a unit from float rounding, moving the frustum's bounding radius depending
    // on camera orientation even though the frustum's shape (and true radius) never changes
    // under a pure rotation. inverse(view) is a rigid transform (condition number 1), so this
    // is exact.
    glm::mat4 invView = glm::inverse(viewMatrix);

    float tanHalfFovY = std::tan(glm::radians(DisplayState::perspectiveFOV) * 0.5f);
    float nearY = tanHalfFovY * DisplayState::nearPlane;
    float nearX = nearY * DisplayState::aspect;
    float farY = tanHalfFovY * DisplayState::farPlane;
    float farX = farY * DisplayState::aspect;

    std::array<glm::vec3, 8> corners = {
        glm::vec3(-nearX, -nearY, -DisplayState::nearPlane),
        glm::vec3(-nearX, nearY, -DisplayState::nearPlane),
        glm::vec3(nearX, -nearY, -DisplayState::nearPlane),
        glm::vec3(nearX, nearY, -DisplayState::nearPlane),
        glm::vec3(-farX, -farY, -DisplayState::farPlane),
        glm::vec3(-farX, farY, -DisplayState::farPlane),
        glm::vec3(farX, -farY, -DisplayState::farPlane),
        glm::vec3(farX, farY, -DisplayState::farPlane),
    };

    for (auto& c : corners) c = glm::vec3(invView * glm::vec4(c, 1.0f));
    return corners;
}

FrustumXZBounds Renderer::getFrustumXZBounds() {
    auto corners = getFrustumCornersWorldSpace();

    FrustumXZBounds bounds{corners[0].x, corners[0].x, corners[0].z, corners[0].z};
    for (const auto& c : corners) {
        bounds.minX = std::min(bounds.minX, c.x);
        bounds.maxX = std::max(bounds.maxX, c.x);
        bounds.minZ = std::min(bounds.minZ, c.z);
        bounds.maxZ = std::max(bounds.maxZ, c.z);
    }
    return bounds;
}

bool Renderer::aabbOutsidePlane(const glm::vec4& plane, const glm::vec3& min,
                                const glm::vec3& max) {
    glm::vec3 positiveVertex = min;
    if (plane.x >= 0) positiveVertex.x = max.x;
    if (plane.y >= 0) positiveVertex.y = max.y;
    if (plane.z >= 0) positiveVertex.z = max.z;

    float distance = plane.x * positiveVertex.x + plane.y * positiveVertex.y +
                     plane.z * positiveVertex.z + plane.w;
    return distance < 0;  // fully behind (outside) this plane
}

bool Renderer::shouldOccludeCull(const glm::vec3& targetPoint) {
    ViewState& view = Registry.get<ViewState>(CameraController::activeCamera);
    glm::vec3 cameraPosition = view.position;
    int numSamples = 8;
    glm::vec3 toChunk = targetPoint - cameraPosition;

    for (int i = 1; i < numSamples; i++) {
        float t = static_cast<float>(i) / numSamples;
        glm::vec3 samplePos = cameraPosition + toChunk * t;
        // Baked heightmap lookup; an unloaded chunk along the ray simply can't occlude.
        float terrainHeight;
        if (Chunks.sampleBakedHeight(samplePos.x, samplePos.z, terrainHeight) &&
            terrainHeight > samplePos.y) {
            return true;
        }
    }
    return false;
}
