#pragma once

#include "core/PrecompiledHeader.hpp"
#include "rendering/RenderLayer.hpp"

// The full set of GL capability/value state a renderable cares about. Every Renderable /
// ChunkRenderable / InstancedRenderable carries one of these (set at construction time from its
// RenderLayer) describing the state GL must be in while it draws. GLState::applyState()
// diffs a given instance against GLState::current instead of bracketing each draw with its own
// enable/disable pair.
struct GLStateFlags {
    GLenum depthFunc = GL_LESS;
    bool depthTest = true;
    bool depthMask = true;
    bool cullFace = true;
    bool depthClamp = false;
    bool blend = true;
    GLenum blendSrcFactor = GL_SRC_ALPHA;
    GLenum blendDstFactor = GL_ONE_MINUS_SRC_ALPHA;
    bool wireframe = false;

    // Layers not listed here render with the defaults above, which match the baseline GL state set
    // once in Game::init().
    static GLStateFlags forLayer(RenderLayer layer) {
        GLStateFlags flags;
        switch (layer) {
            case RenderLayer::Skybox:
                flags.depthFunc = GL_LEQUAL;
                flags.depthMask = false;
                flags.cullFace = false;
                break;
            case RenderLayer::Sun:
                flags.depthMask = false;
                flags.cullFace = false;
                // The sun's orbit (SunHelper::distance/h) can exceed farPlane (e.g. ~1131 units
                // at zenith vs. a 1000-unit far plane), which the hardware would otherwise clip
                // entirely. Clamp instead of clip so it still draws, pinned at max depth, without
                // touching the global far plane tuning.
                flags.depthClamp = true;
                break;
            case RenderLayer::InstancedObjects:
                flags.blend = true;
                flags.blendSrcFactor = GL_SRC_ALPHA;
                flags.blendDstFactor = GL_ONE_MINUS_SRC_ALPHA;
                break;
            case RenderLayer::Objects:
                flags.depthMask = true;
                break;
            case RenderLayer::Terrain:
                flags.depthMask = true;
                break;
            default:
                break;
        }
        return flags;
    }
};

namespace GLState {
inline GLStateFlags current;

// Diffs required (with any DebugState::layerStates[layer] overrides substituted in first) against
// GLState::current and issues only the GL calls needed to bring actual state in line, then updates
// GLState::current to match. Called once per entity in Renderer::renderBatches() instead of
// bracketing each draw with its own enable/disable pair.
void applyState(RenderLayer layer, const GLStateFlags& required);

// Same diff-and-issue logic, for callers with no RenderLayer/debug-override context (frame
// bootstrap, IBL precompute) that want to reset every field to an explicit baseline in one call
// (e.g. a default-constructed GLStateFlags) rather than nudge one or two fields off of
// GLState::current -- for the latter, prefer the per-field setters below.
// force=true skips the diff and unconditionally issues every GL call; only Game::init() should
// need this, since GLState::current's field defaults don't match real GL context defaults and a
// diff there would silently issue nothing.
void applyState(const GLStateFlags& explicitState, bool force = false);

// Single-field diff-and-issue setters: each is a no-op if the value already matches
// GLState::current, and otherwise issues the one GL call needed and updates GLState::current.
// Prefer these over copying GLState::current into a local GLStateFlags and calling applyState()
// just to change one or two fields.
void setDepthTest(bool enabled);
void setDepthFunc(GLenum func);
void setDepthMask(bool enabled);
void setCullFace(bool enabled);
void setDepthClamp(bool enabled);
void setBlend(bool enabled);
void setBlendFunc(GLenum srcFactor, GLenum dstFactor);
void setWireframe(bool enabled);
}  // namespace GLState
