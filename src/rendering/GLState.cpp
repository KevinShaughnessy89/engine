#include "GLState.hpp"

#include "DebugState.hpp"

namespace GLState {

void applyState(RenderLayer layer, const GLStateFlags& required) {
    // DebugState::layerStates always has an entry per layer (eagerly seeded from
    // GLStateFlags::forLayer() at startup), so unless a checkbox in the debug panel's Rendering
    // tab has been touched for this specific layer, `effective` ends up identical to `required`.
    GLStateFlags effective = required;
    const DebugState::LayerDebugState& debug = DebugState::layerStates[layer];
    effective.wireframe = debug.wireframe;
    effective.depthTest = debug.depthTest;
    effective.depthMask = debug.depthMask;
    effective.cullFace = debug.cullFace;
    effective.blend = debug.blend;
    effective.depthClamp = debug.depthClamp;

    applyState(effective);
}

void applyState(const GLStateFlags& explicitState, bool force) {
    const GLStateFlags& effective = explicitState;
    GLStateFlags& current = GLState::current;

    if (force || effective.depthTest != current.depthTest) {
        if (effective.depthTest) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        current.depthTest = effective.depthTest;
    }
    if (force || effective.depthFunc != current.depthFunc) {
        glDepthFunc(effective.depthFunc);
        current.depthFunc = effective.depthFunc;
    }
    if (force || effective.depthMask != current.depthMask) {
        glDepthMask(effective.depthMask ? GL_TRUE : GL_FALSE);
        current.depthMask = effective.depthMask;
    }
    if (force || effective.cullFace != current.cullFace) {
        if (effective.cullFace) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }
        current.cullFace = effective.cullFace;
    }
    if (force || effective.depthClamp != current.depthClamp) {
        if (effective.depthClamp) {
            glEnable(GL_DEPTH_CLAMP);
        } else {
            glDisable(GL_DEPTH_CLAMP);
        }
        current.depthClamp = effective.depthClamp;
    }
    if (force || effective.blend != current.blend) {
        if (effective.blend) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
        current.blend = effective.blend;
    }
    if (effective.blend && (force || effective.blendSrcFactor != current.blendSrcFactor ||
                            effective.blendDstFactor != current.blendDstFactor)) {
        glBlendFunc(effective.blendSrcFactor, effective.blendDstFactor);
        current.blendSrcFactor = effective.blendSrcFactor;
        current.blendDstFactor = effective.blendDstFactor;
    }
    if (force || effective.wireframe != current.wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, effective.wireframe ? GL_LINE : GL_FILL);
        current.wireframe = effective.wireframe;
    }
}

void setDepthTest(bool enabled) {
    if (enabled == current.depthTest) return;
    if (enabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    current.depthTest = enabled;
}

void setDepthFunc(GLenum func) {
    if (func == current.depthFunc) return;
    glDepthFunc(func);
    current.depthFunc = func;
}

void setDepthMask(bool enabled) {
    if (enabled == current.depthMask) return;
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
    current.depthMask = enabled;
}

void setCullFace(bool enabled) {
    if (enabled == current.cullFace) return;
    if (enabled) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }
    current.cullFace = enabled;
}

void setDepthClamp(bool enabled) {
    if (enabled == current.depthClamp) return;
    if (enabled) {
        glEnable(GL_DEPTH_CLAMP);
    } else {
        glDisable(GL_DEPTH_CLAMP);
    }
    current.depthClamp = enabled;
}

void setBlend(bool enabled) {
    if (enabled == current.blend) return;
    if (enabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    current.blend = enabled;
}

void setBlendFunc(GLenum srcFactor, GLenum dstFactor) {
    if (srcFactor == current.blendSrcFactor && dstFactor == current.blendDstFactor) return;
    glBlendFunc(srcFactor, dstFactor);
    current.blendSrcFactor = srcFactor;
    current.blendDstFactor = dstFactor;
}

void setWireframe(bool enabled) {
    if (enabled == current.wireframe) return;
    glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
    current.wireframe = enabled;
}

}  // namespace GLState
