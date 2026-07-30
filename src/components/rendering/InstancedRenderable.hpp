#pragma once

#include "core/PrecompiledHeader.hpp"
#include "rendering/GLState.hpp"
#include "rendering/RenderLayer.hpp"

class InstancedModel;

struct InstancedRenderable {
    InstancedModel* model = nullptr;
    GLuint shaderID = 0;
    RenderLayer layer = RenderLayer::Objects;
    bool shouldRender = false;
    GLStateFlags glStateFlags = GLStateFlags::forLayer(RenderLayer::Objects);

    InstancedRenderable() = default;
    InstancedRenderable(InstancedModel* model, GLuint shaderID, RenderLayer layer,
                        bool shouldRender)
        : model(model),
          shaderID(shaderID),
          layer(layer),
          shouldRender(shouldRender),
          glStateFlags(GLStateFlags::forLayer(layer)) {}
};