#include "EnvironmentManager.hpp"

#include <memory>
#include <tracy/Tracy.hpp>

#include "components/environment/SkyState.hpp"
#include "components/rendering/InstancedRenderable.hpp"
#include "components/rendering/Renderable.hpp"
#include "components/rendering/SunState.hpp"
#include "core/Config.hpp"
#include "core/Core.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "environment/ChunkManager.hpp"
#include "environment/ParticleSystem.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/InstancedModel.hpp"
#include "rendering/ModelLoader.hpp"

EnvironmentManager::EnvironmentManager() {
}

EnvironmentManager::~EnvironmentManager() {
}

void EnvironmentManager::init() {
    particleSystem = new ParticleSystem();

    auto weatherEntity = Registry.create();
    auto& weatherRenderable = Registry.emplace<InstancedRenderable>(
        weatherEntity, particleSystem->getModel(), Display.getParticleShaderID(),
        RenderLayer::Weather, true);

    if (!ModelRegistry.entities["sun"].empty()) {
        Registry.emplace<SunState>(ModelRegistry.getEntity("sun"));
    }
    if (!ModelRegistry.entities["skybox"].empty()) {
        Registry.emplace<SkyState>(ModelRegistry.getEntity("skybox"));
    }

    Chunks.genPipeline.terrainAssetFactory.bakeTreeImposters();
}

void EnvironmentManager::update(double deltaTime) {
    ZoneScoped;
    particleSystem->update(deltaTime);
}