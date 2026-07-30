#pragma once
#include "rendering/GLState.hpp"
#include "rendering/RenderLayer.hpp"

class ChunkModel;

struct ChunkRenderable {
    ChunkModel* model = nullptr;
    GLuint shaderID = std::numeric_limits<int>::max();
    RenderLayer layer = RenderLayer::Objects;
    bool shouldRender = false;
    GLStateFlags glStateFlags = GLStateFlags::forLayer(RenderLayer::Objects);

    ChunkRenderable(ChunkModel* model, GLuint shaderID, RenderLayer layer,
                    bool shouldRender = false)
        : model(model),
          shaderID(shaderID),
          layer(layer),
          shouldRender(shouldRender),
          glStateFlags(GLStateFlags::forLayer(layer)) {}
};