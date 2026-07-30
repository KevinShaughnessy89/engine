#include "DebugPanel.hpp"

#include <tracy/Tracy.hpp>

#include "core/Core.hpp"
#include "core/Game.hpp"
#include "environment/ChunkManager.hpp"
#include "environment/TerrainConfig.hpp"
#include "imgui.h"
#include "rendering/CameraController.hpp"
#include "rendering/DebugState.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/DisplayState.hpp"
#include "rendering/GLState.hpp"
#include "rendering/ModelLoader.hpp"
#include "rendering/RenderLayer.hpp"
#include "rendering/ShadowState.hpp"

namespace {

// Only covers the enum values GLStateFlags actually uses (see rendering/GLState.hpp) -- this is a
// diagnostic label, not a general GL enum decoder.
const char* glEnumToString(GLenum value) {
    switch (value) {
        case GL_NEVER:
            return "GL_NEVER";
        case GL_LESS:
            return "GL_LESS";
        case GL_EQUAL:
            return "GL_EQUAL";
        case GL_LEQUAL:
            return "GL_LEQUAL";
        case GL_GREATER:
            return "GL_GREATER";
        case GL_NOTEQUAL:
            return "GL_NOTEQUAL";
        case GL_GEQUAL:
            return "GL_GEQUAL";
        case GL_ALWAYS:
            return "GL_ALWAYS";
        case GL_ZERO:
            return "GL_ZERO";
        case GL_ONE:
            return "GL_ONE";
        case GL_SRC_ALPHA:
            return "GL_SRC_ALPHA";
        case GL_ONE_MINUS_SRC_ALPHA:
            return "GL_ONE_MINUS_SRC_ALPHA";
        default:
            return "?";
    }
}

void debugGLStateBoolRow(const char* label, bool tracked, bool actual) {
    bool match = tracked == actual;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(tracked ? "true" : "false");
    ImGui::TableSetColumnIndex(2);
    ImGui::TextUnformatted(actual ? "true" : "false");
    ImGui::TableSetColumnIndex(3);
    ImGui::TextColored(match ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                       match ? "OK" : "MISMATCH");
}

void debugGLStateEnumRow(const char* label, GLenum tracked, GLenum actual) {
    bool match = tracked == actual;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(glEnumToString(tracked));
    ImGui::TableSetColumnIndex(2);
    ImGui::TextUnformatted(glEnumToString(actual));
    ImGui::TableSetColumnIndex(3);
    ImGui::TextColored(match ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                       match ? "OK" : "MISMATCH");
}

const char* framebufferStatusToString(GLenum status) {
    switch (status) {
        case GL_FRAMEBUFFER_COMPLETE:
            return "COMPLETE";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            return "INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            return "INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            return "INCOMPLETE_DRAW_BUFFER";
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            return "INCOMPLETE_READ_BUFFER";
        case GL_FRAMEBUFFER_UNSUPPORTED:
            return "UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            return "INCOMPLETE_MULTISAMPLE";
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            return "INCOMPLETE_LAYER_TARGETS";
        default:
            return "UNKNOWN";
    }
}

// Reports glCheckFramebufferStatus for `fbo`, plus the center-texel color (if readBuffer !=
// GL_NONE) and/or depth (if readDepth), and -- if previewTexture is a plain sampler2D (0 to skip,
// e.g. the default framebuffer or a texture-array target ImGui's fixed shader can't sample) -- a
// live thumbnail of it. This is what isolated the alt-enter black screen to combine() rather than
// the scene pass itself; the thumbnail makes that same kind of break visible at a glance instead
// of requiring single-pixel guesswork.
void debugFramebufferRow(const char* label, GLuint fbo, GLenum readBuffer, int width, int height,
                         bool readDepth, GLuint previewTexture = 0) {
    ZoneScoped;
    ZoneText(label, strlen(label));
    GLint prevReadFBO = 0, prevReadBuffer = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
    glGetIntegerv(GL_READ_BUFFER, &prevReadBuffer);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    bool complete = status == GL_FRAMEBUFFER_COMPLETE;

    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    ImGui::TextColored(complete ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                       "[%s]", framebufferStatusToString(status));

    if (readBuffer != GL_NONE) {
        glReadBuffer(readBuffer);
        float pixel[4] = {-1, -1, -1, -1};
        ZoneScopedN("debugFramebufferRow: glReadPixels color (GPU sync)");
        glReadPixels(width / 2, height / 2, 1, 1, GL_RGBA, GL_FLOAT, pixel);
        ImGui::Text("    center pixel color: (%.3f, %.3f, %.3f, %.3f)", pixel[0], pixel[1],
                    pixel[2], pixel[3]);
    }
    if (readDepth) {
        float depth = -1.0f;
        ZoneScopedN("debugFramebufferRow: glReadPixels depth (GPU sync)");
        glReadPixels(width / 2, height / 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        ImGui::Text("    center pixel depth: %.5f", depth);
    }
    if (previewTexture != 0 && width > 0 && height > 0) {
        // Render targets are RGBA16F HDR and origin-bottom-left; flip the V coords so the
        // thumbnail reads right-side-up, and don't worry about values >1 clipping to white --
        // that's expected for HDR bloom sources, not a bug.
        constexpr float kMaxPreviewWidth = 220.0f;
        float previewWidth = std::min(kMaxPreviewWidth, (float)width);
        float previewHeight = previewWidth * ((float)height / (float)width);
        ImGui::Image((ImTextureID)(intptr_t)previewTexture, ImVec2(previewWidth, previewHeight),
                     ImVec2(0, 1), ImVec2(1, 0));
    }
    ImGui::Spacing();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
    glReadBuffer(prevReadBuffer);
}

}  // namespace

void DebugPanel::renderDebugPanel() {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float margin = 40.0f;  // pixels

    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - margin * 2, viewport->Size.y - margin * 2),
                             ImGuiCond_Always);

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + margin, viewport->Pos.y + margin),
                            ImGuiCond_Always);

    ImGui::Begin("Debug Panel", nullptr, flags);

    // Pause/Step: only meaningful once the world is actually running (clocks started), otherwise
    // Game::setPaused() would stop/start FixedRateClocks that Game::updateMainThread() hasn't
    // handed off to yet, racing its own `if (!clocksStarted)` startup.
    if (Game::state == GameState::RUNNING) {
        bool paused = Game::paused;
        if (ImGui::Checkbox("Pause world", &paused)) {
            Game::setPaused(paused);
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!Game::paused);
        if (ImGui::Button("Step frame")) {
            Game::stepRequested = true;
        }
        ImGui::EndDisabled();
    }
    ImGui::Separator();

    if (ImGui::BeginTabBar("DebugTabs")) {
        if (ImGui::BeginTabItem("Rendering")) {
            ImGui::PushID("All");
            if (ImGui::CollapsingHeader("All")) {
                if (ImGui::Checkbox("Wireframe", &DebugState::allState.wireframe)) {
                    for (auto& [layer, state] : DebugState::layerStates) {
                        state.wireframe = DebugState::allState.wireframe;
                    }
                }
                if (ImGui::Checkbox("Depth Test", &DebugState::allState.depthTest)) {
                    for (auto& [layer, state] : DebugState::layerStates) {
                        state.depthTest = DebugState::allState.depthTest;
                    }
                }
                if (ImGui::Checkbox("Depth Mask", &DebugState::allState.depthMask)) {
                    for (auto& [layer, state] : DebugState::layerStates) {
                        state.depthMask = DebugState::allState.depthMask;
                    }
                }
                if (ImGui::Checkbox("Cull Face", &DebugState::allState.cullFace)) {
                    for (auto& [layer, state] : DebugState::layerStates) {
                        state.cullFace = DebugState::allState.cullFace;
                    }
                }
                if (ImGui::Checkbox("Blend", &DebugState::allState.blend)) {
                    for (auto& [layer, state] : DebugState::layerStates) {
                        state.blend = DebugState::allState.blend;
                    }
                }
                if (ImGui::Checkbox("Depth Clamp", &DebugState::allState.depthClamp)) {
                    for (auto& [layer, state] : DebugState::layerStates) {
                        state.depthClamp = DebugState::allState.depthClamp;
                    }
                }
            }
            ImGui::PopID();

            for (RenderLayer layer : allRenderLayers) {
                DebugState::LayerDebugState& state = DebugState::layerStates[layer];

                ImGui::PushID(static_cast<int>(layer));
                if (ImGui::CollapsingHeader(renderLayerToString(layer))) {
                    ImGui::Checkbox("Wireframe", &state.wireframe);
                    ImGui::Checkbox("Depth Test", &state.depthTest);
                    ImGui::Checkbox("Depth Mask", &state.depthMask);
                    ImGui::Checkbox("Cull Face", &state.cullFace);
                    ImGui::Checkbox("Blend", &state.blend);
                    ImGui::Checkbox("Depth Clamp", &state.depthClamp);
                }
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Sky")) {
            SunState& sun = Registry.get<SunState>(ModelRegistry.getEntity("sun"));

            if (!sunStateCached) {
                cachedSunState = sun;
                sunStateCached = true;
                // Freeze SunHelper::updateSunState() from here on, so it stops recomputing sun
                // every frame and both these edits and cachedSunState's baseline actually hold.
                DebugState::sunManualOverride = true;
            }

            ImGui::DragFloat3("Position", &sun.position.x);
            ImGui::DragFloat3("Direction", &sun.direction.x);

            ImGui::Text("Sun Color");
            ImGui::SliderFloat("Sun R", &sun.sunColor.r, 0.0f, 1.0f);
            ImGui::SliderFloat("Sun G", &sun.sunColor.g, 0.0f, 1.0f);
            ImGui::SliderFloat("Sun B", &sun.sunColor.b, 0.0f, 1.0f);

            ImGui::Text("Top Color");
            ImGui::SliderFloat("Top R", &sun.topColor.r, 0.0f, 1.0f);
            ImGui::SliderFloat("Top G", &sun.topColor.g, 0.0f, 1.0f);
            ImGui::SliderFloat("Top B", &sun.topColor.b, 0.0f, 1.0f);

            ImGui::Text("Horizon Color");
            ImGui::SliderFloat("Horizon R", &sun.horizonColor.r, 0.0f, 1.0f);
            ImGui::SliderFloat("Horizon G", &sun.horizonColor.g, 0.0f, 1.0f);
            ImGui::SliderFloat("Horizon B", &sun.horizonColor.b, 0.0f, 1.0f);

            ImGui::Text("Bottom Color");
            ImGui::SliderFloat("Bottom R", &sun.bottomColor.r, 0.0f, 1.0f);
            ImGui::SliderFloat("Bottom G", &sun.bottomColor.g, 0.0f, 1.0f);
            ImGui::SliderFloat("Bottom B", &sun.bottomColor.b, 0.0f, 1.0f);

            ImGui::Text("Sky Color Average");
            ImGui::SliderFloat("Avg R", &sun.skyColorAverage.r, 0.0f, 1.0f);
            ImGui::SliderFloat("Avg G", &sun.skyColorAverage.g, 0.0f, 1.0f);
            ImGui::SliderFloat("Avg B", &sun.skyColorAverage.b, 0.0f, 1.0f);

            ImGui::SliderFloat("Intensity", &sun.intensity, 0.0f, 50.0f);
            ImGui::SliderFloat("Ambient Strength", &sun.ambientStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("Time", &sun.time, 0.0f, 1.0f);
            ImGui::SliderFloat("Elevation", &sun.elevation, -1.0f, 1.0f);

            if (ImGui::Button("Revert")) {
                sun = cachedSunState;
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Sky")) {
                sun.topColor = glm::vec3(0.0f);
                sun.horizonColor = glm::vec3(0.0f);
                sun.bottomColor = glm::vec3(0.0f);
                sun.skyColorAverage = glm::vec3(0.0f);
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Terrain")) {
            ImGui::Checkbox("Generate Foliage", &DebugState::shouldGenerateFoliage);
            ImGui::Separator();
            if (ImGui::BeginTabBar("TerrainSubTabs")) {
                if (ImGui::BeginTabItem("Rendering")) {
                    if (pendingTriangleCount < 0)
                        pendingTriangleCount = TerrainConfig::triangleCount;

                    ImGui::SliderInt("Triangle Count", &pendingTriangleCount, 2, 512);
                    if (ImGui::Button("Apply")) {
                        TerrainConfig::triangleCount = pendingTriangleCount;
                        Chunks.regenerateTerrain();
                    }

                    ImGui::SliderFloat("LOD 1 Distance", &TerrainConfig::lod1Distance, 0.0f,
                                       TerrainConfig::lod2Distance);
                    ImGui::SliderFloat("LOD 2 Distance", &TerrainConfig::lod2Distance,
                                       TerrainConfig::lod1Distance, 8192.0f);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Noise")) {
                    // These all feed HeightMapGenerator::getHeightAt() directly (see
                    // DebugState.hpp) rather than being blended per-biome, so edits apply uniformly
                    // across the whole world instead of only shaping newly-streamed terrain near a
                    // particular biome.
                    ImGui::SeparatorText("Base Noise");
                    ImGui::InputFloat("Lowland Frequency", &DebugState::lowlandFrequency, 0.0f,
                                      0.0f, "%.7f");
                    ImGui::InputFloat("Mountain Frequency", &DebugState::mountainFrequency, 0.0f,
                                      0.0f, "%.9f");
                    ImGui::InputFloat("Warp Frequency", &DebugState::warpFrequency, 0.0f, 0.0f,
                                      "%.5f");
                    ImGui::InputFloat("Warp Strength", &DebugState::warpStrength, 0.0f, 0.0f,
                                      "%.2f");

                    ImGui::SeparatorText("Biome Blend");
                    ImGui::InputFloat("Moisture Frequency", &DebugState::moistureFrequency, 0.0f,
                                      0.0f, "%.7f");
                    ImGui::InputFloat("Temperature Frequency", &DebugState::temperatureFrequency,
                                      0.0f, 0.0f, "%.7f");
                    ImGui::InputFloat("Biome Sigma", &DebugState::biomeSigma, 0.0f, 0.0f, "%.3f");

                    ImGui::SeparatorText("Mountain Shape");
                    ImGui::InputFloat("Lowland/Mountain Split", &DebugState::biomeAmplitudeSplit,
                                      0.0f, 0.0f, "%.1f");
                    ImGui::InputFloat("Mountain Sharpness", &DebugState::mountainSharpness, 0.0f,
                                      0.0f, "%.2f");
                    ImGui::InputFloat("Mountain Mask Threshold Low",
                                      &DebugState::mountainMaskThresholdLow, 0.0f, 0.0f, "%.1f");
                    ImGui::InputFloat("Mountain Mask Threshold High",
                                      &DebugState::mountainMaskThresholdHigh, 0.0f, 0.0f, "%.1f");
                    ImGui::InputFloat("Mountain Amplitude Scale", &DebugState::debugAmplitudeScale,
                                      0.0f, 0.0f, "%.2f");

                    ImGui::SeparatorText("Mountain Detail");
                    ImGui::InputFloat("Mountain Micro Cell Size",
                                      &DebugState::mountainMicroCellSize, 0.0f, 0.0f, "%.1f");
                    ImGui::InputFloat("Mountain Micro Smoothing",
                                      &DebugState::mountainMicroSmoothing, 0.0f, 0.0f, "%.3f");
                    ImGui::InputFloat("Mountain Micro Sharpness",
                                      &DebugState::mountainMicroSharpness, 0.0f, 0.0f, "%.2f");
                    ImGui::InputFloat("Mountain Detail Strength",
                                      &DebugState::mountainDetailStrength, 0.0f, 0.0f, "%.2f");
                    ImGui::InputFloat("Mountain Micro Ridge Cell Size",
                                      &DebugState::mountainMicroRidgeCellSize, 0.0f, 0.0f, "%.1f");
                    ImGui::InputFloat("Mountain Micro Ridge Sharpness",
                                      &DebugState::mountainMicroRidgeSharpness, 0.0f, 0.0f, "%.2f");
                    ImGui::InputFloat("Mountain Micro Ridge Strength",
                                      &DebugState::mountainMicroRidgeStrength, 0.0f, 0.0f, "%.2f");

                    ImGui::Spacing();
                    if (ImGui::Button("Apply")) {
                        Chunks.regenerateTerrain();
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Camera")) {
            KinematicBody& camBody =
                CameraController::getKinematicBody(CameraController::activeCamera);
            ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", camBody.position.x,
                        camBody.position.y, camBody.position.z);
            ImGui::DragFloat("Camera Speed", &camBody.speed, 1.0f, 0.0f, 2000.0f);

            ImGui::Spacing();
            ImGui::Separator();

            bool changed = false;
            changed |= ImGui::DragFloat("FOV (degrees)", &DisplayState::perspectiveFOV, 0.5f, 1.0f,
                                        179.0f);
            changed |= ImGui::DragFloat("Near Plane", &DisplayState::nearPlane, 0.01f, 0.001f,
                                        DisplayState::farPlane - 0.01f);
            changed |= ImGui::DragFloat("Far Plane", &DisplayState::farPlane, 1.0f,
                                        DisplayState::nearPlane + 0.01f, 100000.0f);
            changed |= ImGui::DragFloat("Aspect", &DisplayState::aspect, 0.01f, 0.1f, 4.0f);

            if (changed) {
                Display.updateProjectionMatrices((int)DisplayState::screenWidth,
                                                 (int)DisplayState::screenHeight);
            }
            if (ImGui::Button("Reset aspect to window")) {
                DisplayState::aspect = DisplayState::screenWidth / DisplayState::screenHeight;
                Display.updateProjectionMatrices((int)DisplayState::screenWidth,
                                                 (int)DisplayState::screenHeight);
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Stats")) {
            ImGui::Text("Frames per second: %.2f", DebugState::currentFPS);
            ImGui::SameLine();
            ImGui::Checkbox("Overlay", &DebugState::showFpsOverlay);

            Renderer& br = Display.getRenderer();
            ImGui::Text("Renderer window size: %d x %d", br.windowWidth, br.windowHeight);
            ImGui::Text("DisplayState aspect: %.4f (screen %.0f x %.0f)", DisplayState::aspect,
                        DisplayState::screenWidth, DisplayState::screenHeight);
            int fbWidth, fbHeight, winWidth, winHeight;
            GLFWwindow* glfwWindow = glfwGetCurrentContext();
            glfwGetFramebufferSize(glfwWindow, &fbWidth, &fbHeight);
            glfwGetWindowSize(glfwWindow, &winWidth, &winHeight);
            ImGui::Text("glfw framebuffer size: %d x %d", fbWidth, fbHeight);
            ImGui::Text("glfw window size: %d x %d", winWidth, winHeight);
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            ImGui::Text("GL_VIEWPORT: x=%d y=%d w=%d h=%d", viewport[0], viewport[1], viewport[2],
                        viewport[3]);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Framebuffers")) {
            ZoneScopedN("DebugPanel: Framebuffers tab (glReadPixels x6)");
            ImGui::Spacing();

            Renderer& br = Display.getRenderer();

            debugFramebufferRow(
                "1. Scene pass color output  (sceneFBO, color attachment 0 / sceneTex)",
                br.sceneFBO, GL_COLOR_ATTACHMENT0, br.windowWidth, br.windowHeight,
                /*readDepth=*/true, /*previewTexture=*/br.sceneTex);

            debugFramebufferRow(
                "2. Scene pass bloom-bright output  (sceneFBO, color attachment 1 / brightTex)",
                br.sceneFBO, GL_COLOR_ATTACHMENT1, br.windowWidth, br.windowHeight,
                /*readDepth=*/false, /*previewTexture=*/br.brightTex);

            debugFramebufferRow("3. Bloom blur buffer A  (pingpongFBO[0] / pingpongTex[0])",
                                br.pingpongFBO[0], GL_COLOR_ATTACHMENT0, br.windowWidth,
                                br.windowHeight,
                                /*readDepth=*/false, /*previewTexture=*/br.pingpongTex[0]);

            debugFramebufferRow("4. Bloom blur buffer B  (pingpongFBO[1] / pingpongTex[1])",
                                br.pingpongFBO[1], GL_COLOR_ATTACHMENT0, br.windowWidth,
                                br.windowHeight,
                                /*readDepth=*/false, /*previewTexture=*/br.pingpongTex[1]);

            debugFramebufferRow("5. Final composited frame  (default framebuffer, after combine())",
                                0, GL_BACK, br.windowWidth, br.windowHeight, /*readDepth=*/false);

            ImGui::Spacing();
            ImGui::Separator();

            debugFramebufferRow(
                "Shadow depth map  (shadowMapFBO / shadowTextureArray, last cascade layer bound)",
                ShadowState::shadowMapFBO, GL_NONE, ShadowState::shadowMapSize,
                ShadowState::shadowMapSize, /*readDepth=*/true);

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("GL State")) {
            const GLStateFlags& tracked = GLState::current;

            GLboolean actualDepthTest = glIsEnabled(GL_DEPTH_TEST);
            GLint actualDepthFunc = 0;
            glGetIntegerv(GL_DEPTH_FUNC, &actualDepthFunc);
            GLboolean actualDepthMask = GL_FALSE;
            glGetBooleanv(GL_DEPTH_WRITEMASK, &actualDepthMask);
            GLboolean actualCullFace = glIsEnabled(GL_CULL_FACE);
            GLboolean actualDepthClamp = glIsEnabled(GL_DEPTH_CLAMP);
            GLboolean actualBlend = glIsEnabled(GL_BLEND);
            GLint actualBlendSrc = 0, actualBlendDst = 0;
            glGetIntegerv(GL_BLEND_SRC_RGB, &actualBlendSrc);
            glGetIntegerv(GL_BLEND_DST_RGB, &actualBlendDst);
            GLint actualPolygonMode[2] = {0, 0};
            glGetIntegerv(GL_POLYGON_MODE, actualPolygonMode);
            bool actualWireframe = actualPolygonMode[0] == GL_LINE;

            if (ImGui::BeginTable("GLStateTable", 4,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Flag");
                ImGui::TableSetupColumn("Tracked (GLState::current)");
                ImGui::TableSetupColumn("Actual (hardware)");
                ImGui::TableSetupColumn("");
                ImGui::TableHeadersRow();

                debugGLStateBoolRow("GL_DEPTH_TEST", tracked.depthTest, actualDepthTest);
                debugGLStateEnumRow("GL_DEPTH_FUNC", tracked.depthFunc, (GLenum)actualDepthFunc);
                debugGLStateBoolRow("GL_DEPTH_WRITEMASK", tracked.depthMask, actualDepthMask);
                debugGLStateBoolRow("GL_CULL_FACE", tracked.cullFace, actualCullFace);
                debugGLStateBoolRow("GL_DEPTH_CLAMP", tracked.depthClamp, actualDepthClamp);
                debugGLStateBoolRow("GL_BLEND", tracked.blend, actualBlend);
                debugGLStateEnumRow("GL_BLEND_SRC_RGB", tracked.blendSrcFactor,
                                    (GLenum)actualBlendSrc);
                debugGLStateEnumRow("GL_BLEND_DST_RGB", tracked.blendDstFactor,
                                    (GLenum)actualBlendDst);
                debugGLStateBoolRow("Wireframe (GL_POLYGON_MODE == GL_LINE)", tracked.wireframe,
                                    actualWireframe);

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Render Stats")) {
            if (ImGui::BeginTable("RenderStatsTable", 3,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Layer");
                ImGui::TableSetupColumn("Submitted");
                ImGui::TableSetupColumn("Rendered");
                ImGui::TableHeadersRow();

                for (RenderLayer layer : allRenderLayers) {
                    const DebugState::LayerRenderStats& stats = DebugState::layerRenderStats[layer];
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(renderLayerToString(layer));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", stats.submitted);
                    ImGui::TableSetColumnIndex(2);
                    bool allCulled = stats.submitted > 0 && stats.rendered == 0;
                    ImGui::TextColored(
                        allCulled ? ImVec4(1.0f, 0.7f, 0.3f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
                        "%d", stats.rendered);
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Shadow")) {
            if (pendingCascadeCount < 0) pendingCascadeCount = ShadowState::shadowCascadeCount;

            ImGui::SliderInt("Cascade Count", &pendingCascadeCount, 1, 8);
            if (ImGui::Button("Apply")) {
                Display.getRenderer().setCascadeCount(pendingCascadeCount);
                pendingCascadeCount = ShadowState::shadowCascadeCount;
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void DebugPanel::renderOverlay() {
    if (!DebugState::showFpsOverlay) return;

    char buf[32];
    snprintf(buf, sizeof(buf), "%.0f FPS", DebugState::currentFPS);

    ImGui::GetForegroundDrawList()->AddText(ImVec2(10.0f, 10.0f), IM_COL32(0, 0, 0, 255), buf);
}