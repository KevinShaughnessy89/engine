#pragma once

#include <limits>

#include "core/PrecompiledHeader.hpp"
#include "rendering/GLState.hpp"
#include "rendering/RenderLayer.hpp"
#include "rendering/Texture.hpp"
#include "rendering/UniformData.hpp"

class ShaderProgram;
class Model;

struct Renderable {
    Model* model = nullptr;
    GLuint shaderID = std::numeric_limits<int>::max();
    RenderLayer layer = RenderLayer::Objects;
    bool shouldRender = false;
    GLStateFlags glStateFlags = GLStateFlags::forLayer(RenderLayer::Objects);

    Renderable(Model* model, GLuint shaderID, RenderLayer layer, bool shouldRender = false)
        : model(model),
          shaderID(shaderID),
          layer(layer),
          shouldRender(shouldRender),
          glStateFlags(GLStateFlags::forLayer(layer)) {}

    Renderable() = default;
};