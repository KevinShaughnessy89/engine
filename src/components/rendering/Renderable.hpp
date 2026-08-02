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
    // Fixed per-asset yaw correction (degrees) between this model's baked-in facing direction and
    // CameraConstants::WORLD_FORWARD_AXIS -- applied on top of KinematicBody::orientation in
    // UniformUpdater::calculateModelMatrix. Set from models.json's "forwardOffsetDegrees".
    float forwardOffsetDegrees = 0.0f;

    Renderable(Model* model, GLuint shaderID, RenderLayer layer, bool shouldRender = false,
              float forwardOffsetDegrees = 0.0f)
        : model(model),
          shaderID(shaderID),
          layer(layer),
          shouldRender(shouldRender),
          glStateFlags(GLStateFlags::forLayer(layer)),
          forwardOffsetDegrees(forwardOffsetDegrees) {}

    Renderable() = default;
};