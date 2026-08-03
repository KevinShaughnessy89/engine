#include "UniformUpdater.hpp"

#include "CameraConstants.hpp"
#include "ShaderProgram.hpp"
#include "components/environment/SkyState.hpp"
#include "components/physics/PhysicsComponents.hpp"
#include "components/rendering/AnimationState.hpp"
#include "components/rendering/SunState.hpp"
#include "components/rendering/TerrainTreeImposter.hpp"
#include "components/rendering/ViewState.hpp"
#include "core/Core.hpp"
#include "environment/EnvironmentState.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/DisplayState.hpp"
#include "rendering/InstancedModel.hpp"
#include "rendering/ModelLoader.hpp"
#include "rendering/OcclusionMapGenerator.hpp"
#include "rendering/SceneState.hpp"
#include "rendering/ShadowState.hpp"
#include "rendering/Texture.hpp"
#include "rendering/sky/SunHelper.hpp"

namespace UniformUpdater {

glm::mat4 calculateModelMatrix(entt::entity entity) {
    if (Registry.all_of<SunState>(entity)) {
        auto& sunState = Registry.get<SunState>(entity);
        auto model = glm::mat4(1.f);
        model = glm::translate(model, sunState.position);
        model = glm::scale(model, glm::vec3(SunHelper::radius));
        return model;
    }
    if (Registry.all_of<KinematicBody>(entity)) {
        auto& body = Registry.get<KinematicBody>(entity);
        glm::mat4 T = glm::translate(glm::mat4(1.0f), body.position);
        glm::mat4 R = glm::mat4_cast(body.orientation);
        if (auto* renderable = Registry.try_get<Renderable>(entity)) {
            R = R * glm::rotate(glm::mat4(1.0f), glm::radians(renderable->forwardOffsetDegrees),
                                CameraConstants::WORLD_UP_AXIS);
        }
        return T * R;
    }
    if (Registry.all_of<Position, Orientation>(entity)) {
        auto& position = Registry.get<Position>(entity).current;
        auto& orientation = Registry.get<Orientation>(entity);
        glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 R = glm::mat4(orientation.rotationMatrix);
        return T * R;
    } else {
        return glm::mat4(1.f);
    }
}

static int noSpam = 0;
void updateUniforms(ShaderProgram* shader, entt::entity entity) {
    auto uniforms = shader->uniformLocationCache;
    auto& view = Registry.get<ViewState>(CameraController::activeCamera);
    auto& sunState = Registry.get<SunState>(ModelRegistry.getEntity("sun"));

    auto [renderable, instancedRenderable, chunkRenderable] =
        Registry.try_get<Renderable, InstancedRenderable, ChunkRenderable>(entity);
    auto* terrainTreeImposter = Registry.try_get<TerrainTreeImposter>(entity);

    for (const auto& [name, location] : uniforms) {
        if (location == -1 && noSpam % 100 == 0) {
            noSpam++;
            std::cerr
                << "Warning: uniform '" << name
                << "'attempting to be updated but doesn't exist or is not used in shader program."
                << std::endl;
            continue;
        }

        if (name == toCString(UniformName::View)) {
            updateUniform(UniformData(UniformName::View,
                                      glm::lookAtRH(view.position, view.position + view.forward,
                                                    CameraConstants::WORLD_UP_AXIS)),
                          shader);
        }
        if (name == toCString(UniformName::Projection)) {
            updateUniform(UniformData(UniformName::Projection, DisplayState::perspectiveMatrix),
                          shader);
        }
        if (name == toCString(UniformName::ViewPos)) {
            updateUniform(UniformData(UniformName::ViewPos, view.position), shader);
        }
        if (name == toCString(UniformName::ScreenWidth)) {
            updateUniform(UniformData(UniformName::ScreenWidth, DisplayState::screenWidth), shader);
        }
        if (name == toCString(UniformName::ScreenHeight)) {
            updateUniform(UniformData(UniformName::ScreenHeight, DisplayState::screenHeight),
                          shader);
        }
        if (name == toCString(UniformName::SunPosition)) {
            updateUniform(UniformData(UniformName::SunPosition, sunState.position), shader);
        }
        if (name == toCString(UniformName::LightPos)) {
            updateUniform(UniformData(UniformName::LightPos, sunState.position), shader);
        }
        if (name == toCString(UniformName::LightColor)) {
            updateUniform(UniformData(UniformName::LightColor, sunState.sunColor), shader);
        }
        if (name == toCString(UniformName::SunColor)) {
            updateUniform(UniformData(UniformName::SunColor, sunState.sunColor), shader);
        }
        if (name == toCString(UniformName::AmbientStrength)) {
            updateUniform(UniformData(UniformName::AmbientStrength, sunState.ambientStrength),
                          shader);
        }
        if (name == toCString(UniformName::Intensity)) {
            updateUniform(UniformData(UniformName::Intensity, sunState.intensity), shader);
        }
        if (name == toCString(UniformName::SkyColorAverage)) {
            updateUniform(UniformData(UniformName::SkyColorAverage, sunState.skyColorAverage),
                          shader);
        }
        if (name == toCString(UniformName::SunTopColor)) {
            updateUniform(UniformData(UniformName::SunTopColor, sunState.topColor), shader);
        }
        if (name == toCString(UniformName::SunHorizonColor)) {
            updateUniform(UniformData(UniformName::SunHorizonColor, sunState.horizonColor), shader);
        }
        if (name == toCString(UniformName::SunBottomColor)) {
            updateUniform(UniformData(UniformName::SunBottomColor, sunState.bottomColor), shader);
        }
        if (name == toCString(UniformName::GlobalMinHeight)) {
            updateUniform(
                UniformData(UniformName::GlobalMinHeight, EnvironmentState::globalMinHeight),
                shader);
        }
        if (name == toCString(UniformName::GlobalMaxHeight)) {
            updateUniform(
                UniformData(UniformName::GlobalMaxHeight, EnvironmentState::globalMaxHeight),
                shader);
        }
        // Bake-time snapshot, not live world bounds: keeps worldUV aligned with the baked maps.
        if (name == toCString(UniformName::WorldMin)) {
            updateUniform(UniformData(UniformName::WorldMin, EnvironmentState::normalMapMin),
                          shader);
        }
        if (name == toCString(UniformName::WorldMax)) {
            updateUniform(UniformData(UniformName::WorldMax, EnvironmentState::normalMapMax),
                          shader);
        }
        if (name == toCString(UniformName::ShadowMapArray)) {
            glActiveTexture(GL_TEXTURE0 + ShadowState::shadowMapTextureUnit);
            glBindTexture(GL_TEXTURE_2D_ARRAY, ShadowState::shadowTextureArray);
            shader->setInt(toCString(UniformName::ShadowMapArray),
                           ShadowState::shadowMapTextureUnit);

            updateUBO(ShadowState::cascadeUBO, &ShadowState::cascadeData,
                      sizeof(ShadowState::cascadeData));
        }
        if (name == toCString(UniformName::GridRes)) {
            if (terrainTreeImposter) {
                updateUniform(UniformData(UniformName::GridRes, terrainTreeImposter->gridRes),
                              shader);
            }
        }
        if (name == toCString(UniformName::SkinSlot)) {
            if (auto* animationState = Registry.try_get<AnimationState>(entity)) {
                updateUniform(UniformData(UniformName::SkinSlot, animationState->skinSlot), shader);
            }
        }
        if (name == toCString(UniformName::Tint)) {
            updateUniform(UniformData(UniformName::Tint, glm::vec3(1.0f)), shader);
        }
        if (name == toCString(UniformName::Model)) {
            if (renderable) {
                updateUniform(UniformData(UniformName::Model, calculateModelMatrix(entity)),
                              shader);
            } else if (chunkRenderable) {
                updateUniform(UniformData(UniformName::Model, glm::mat4(1.f)), shader);
            } else if (instancedRenderable) {
                // Instances are positioned per-instance via the instance buffer, not a single
                // model uniform, so there's nothing to set here.
            }
        }
    }
}

void updateTextureUniform(ShaderProgram* shader, entt::entity entity) {
    if (shader->uniformLocationCache.contains(toCString(UniformName::IrradianceMap))) {
        glActiveTexture(GL_TEXTURE0 + SceneState::irradianceMapTextureUnit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, SceneState::irradianceMap);
        shader->setInt(toCString(UniformName::IrradianceMap), SceneState::irradianceMapTextureUnit);
    }
    if (shader->uniformLocationCache.contains(toCString(UniformName::SSAOBlurTexture))) {
        glActiveTexture(GL_TEXTURE0 + OcclusionMapGenerator::ssaoBlurTextureUnit);
        glBindTexture(GL_TEXTURE_2D, OcclusionMapGenerator::ssaoBlurTexture);
        shader->setInt(toCString(UniformName::SSAOBlurTexture),
                       OcclusionMapGenerator::ssaoBlurTextureUnit);
    }
}

void updateNormalMapUniform(ShaderProgram* shader, InstancedMesh& mesh) {
    if (!shader->uniformLocationCache.contains(toCString(UniformName::UseNormalMap))) return;
    updateUniform(UniformData(UniformName::UseNormalMap, mesh.hasNormalMap ? 1 : 0), shader);
}

void updateAlphaMapUniform(ShaderProgram* shader, InstancedMesh& mesh) {
    if (!shader->uniformLocationCache.contains(toCString(UniformName::UseAlphaMap))) return;
    updateUniform(UniformData(UniformName::UseAlphaMap, mesh.hasAlphaMap ? 1 : 0), shader);
}

void updateNormalMapUniform(ShaderProgram* shader, Mesh& mesh) {
    if (!shader->uniformLocationCache.contains(toCString(UniformName::UseNormalMap))) return;
    updateUniform(UniformData(UniformName::UseNormalMap, mesh.hasNormalMap ? 1 : 0), shader);
}

void updateSpecularMapUniform(ShaderProgram* shader, Mesh& mesh) {
    if (!shader->uniformLocationCache.contains(toCString(UniformName::UseSpecularMap))) return;
    updateUniform(UniformData(UniformName::UseSpecularMap, mesh.hasSpecularMap ? 1 : 0), shader);
}

void updateEmissiveMapUniform(ShaderProgram* shader, Mesh& mesh) {
    if (!shader->uniformLocationCache.contains(toCString(UniformName::UseEmissiveMap))) return;
    updateUniform(UniformData(UniformName::UseEmissiveMap, mesh.hasEmissiveMap ? 1 : 0), shader);
}

void updateShininessMapUniform(ShaderProgram* shader, Mesh& mesh) {
    if (!shader->uniformLocationCache.contains(toCString(UniformName::UseShininessMap))) return;
    updateUniform(UniformData(UniformName::UseShininessMap, mesh.hasShininessMap ? 1 : 0), shader);
}

void updateUBO(GLuint ubo, const void* data, size_t size) {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void updateUniform(const UniformData& uniform, ShaderProgram* shader) {
    // printVariant(uniform.name, uniform.value);
    const char* name = toCString(uniform.name);
    if (std::holds_alternative<float>(uniform.value)) {
        shader->setFloat(name, std::get<float>(uniform.value));
    }
    if (std::holds_alternative<int>(uniform.value)) {
        shader->setInt(name, std::get<int>(uniform.value));
    }
    if (std::holds_alternative<glm::vec2>(uniform.value)) {
        shader->setVec2(name, std::get<glm::vec2>(uniform.value));
    }
    if (std::holds_alternative<glm::vec3>(uniform.value)) {
        shader->setVec3(name, std::get<glm::vec3>(uniform.value));
    }
    if (std::holds_alternative<glm::vec4>(uniform.value)) {
        shader->setVec4(name, std::get<glm::vec4>(uniform.value));
    }
    if (std::holds_alternative<glm::mat3>(uniform.value)) {
        shader->setMat3(name, std::get<glm::mat3>(uniform.value));
    }
    if (std::holds_alternative<glm::mat4>(uniform.value)) {
        shader->setMat4(name, std::get<glm::mat4>(uniform.value));
    }
}

}  // namespace UniformUpdater
