#pragma once

#include "UniformData.hpp"
#include "components/rendering/ChunkRenderable.hpp"
#include "components/rendering/InstancedRenderable.hpp"
#include "components/rendering/Renderable.hpp"
#include "entt-main/src/entt/entity/fwd.hpp"

class ShaderProgram;
class InstancedMesh;

namespace UniformUpdater {
inline int firstFreeTextureID = 0;

void updateUniforms(ShaderProgram* shader, entt::entity entity);
void updateTextureUniform(ShaderProgram* shader, entt::entity entity);
void updateNormalMapUniform(ShaderProgram* shader, InstancedMesh& mesh);
void updateAlphaMapUniform(ShaderProgram* shader, InstancedMesh& mesh);
glm::mat4 calculateModelMatrix(entt::entity entity);

void updateUniform(const UniformData& uniform, ShaderProgram* shader);
void updateUBO(GLuint ubo, const void* data, size_t size);
}  // namespace UniformUpdater
