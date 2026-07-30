#include "ProjectileManager.hpp"

#include "collision/CollisionType.hpp"
#include "collision/shapes/ColliderFactory.hpp"
#include "collision/shapes/OBB.hpp"
#include "collision/shapes/Sphere.hpp"
#include "components/physics/JustCreated.hpp"
#include "components/physics/PhysicsComponents.hpp"
#include "components/physics/KinematicBody.hpp"
#include "components/rendering/ViewState.hpp"
#include "components/rendering/Renderable.hpp"
#include "core/CommandBuffer.hpp"
#include "core/Core.hpp"
#include "core/Game.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "environment/ChunkManager.hpp"
#include "imgui.h"
#include "rendering/CameraController.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/Model.hpp"
#include "rendering/ModelLoader.hpp"
#include "rendering/RenderLayer.hpp"


ProjectileManager::ProjectileManager() {
    setDefaultKeyBindings();
}

void ProjectileManager::init() {
}

ProjectileManager::~ProjectileManager() = default;

void ProjectileManager::addProjectileAsset(const ProjectileAsset& projectile) {
    projectileTemplates.push_back(projectile);
}

void ProjectileManager::fireProjectile() {
    for (const auto& projectileType : projectileTemplates)
        if (projectileType.UID == currentProjectileType) {
            const auto& cameraBody =
                CameraController::getKinematicBody(CameraController::activeCamera);
            const auto& cameraView =
                CameraController::getViewState(CameraController::activeCamera);

            const auto cameraPos = cameraBody.position;
            const auto cameraForward = cameraView.forward;

            std::cout << "Projectile fired - Position: " << glm::to_string(cameraPos)
                      << ", Direction: " << glm::to_string(cameraForward) << std::endl;

            Commands.push(ProjectileSpawnCommand{
                .modelFilepath = projectileType.filepath,
                .position = cameraPos,
                .velocity = cameraForward * projectileType.force,
                .acceleration = glm::vec3(0.f),
                .inverseMass = projectileType.inverseMass,
                .force = projectileType.force,
                .linearDamping = 0.99f,
                .angularDamping = 0.99f,
                .boundingVolume = projectileType.boundingVolume,
            });
        }
}

void ProjectileManager::createProjectile(const ProjectileSpawnCommand& command) {
    std::string modelName = command.boundingVolume == CollisionType::Sphere ? "sphere" : "box";
    if (ModelRegistry.entities[modelName].empty()) {
        std::cout << "Warning: no \"" << modelName << "\" model loaded; skipping projectile spawn."
                  << std::endl;
        return;
    }
    Model* model = Registry.get<Renderable>(ModelRegistry.entities[modelName].front()).model;

    entt::entity e = Registry.create();

    Registry.emplace<JustCreated>(e);
    Registry.emplace<ShapeData>(e);
    Registry.emplace<Position>(e, command.position, command.position);
    Registry.emplace<Velocity>(e, command.velocity, glm::vec3(0.0f));
    Registry.emplace<Acceleration>(e, command.acceleration);
    Registry.emplace<InverseMass>(e, command.inverseMass);
    Registry.emplace<Renderable>(e, model, Display.getModelShaderID(), RenderLayer::Projectiles,
                                 true);
    Registry.emplace<Force>(e, command.force);
    Registry.emplace<Sleep>(e, Sleep{});
    Registry.emplace<Torque>(e, glm::vec3(0.0f), glm::vec3(0.0f));
    Registry.emplace<Inertia>(e, glm::mat3(1.0f), glm::mat3(1.0f), glm::mat3(1.0f),
                              glm::translate(glm::mat4(1.0f), command.position));
    Registry.emplace<Damping>(e, command.linearDamping, command.angularDamping);
    Registry.emplace<Orientation>(e, glm::mat3(1.0f),
                                  glm::translate(glm::mat4(1.0f), command.position));

    switch (command.boundingVolume) {
        case CollisionType::Sphere: {
            SphereData& data = ColliderFactory::addSphere(Registry, e);
            Sphere::defineRadialLength(data, model->calculateGeometricData());
            Sphere::defineCenterPosition(data, command.position);
            break;
        }
        case CollisionType::OBB: {
            OBBData& data = ColliderFactory::addOBB(Registry, e);
            OBB::defineBoundingVolume(data, model->calculateGeometricData());
            break;
        }
    }

    projectileEntityRegistry.push_back(e);
    projectileModels.emplace_back(e, model);
}

void ProjectileManager::switchProjectileType() {
    static int currentIndex = 0;
    int newIndex = ++currentIndex % availableProjectileTypes.size();
    currentProjectileType = availableProjectileTypes[newIndex];
    currentProjectileTypeIndex = newIndex;
    std::cout << "Switched to projectile type: " << currentProjectileType << std::endl;
}

void ProjectileManager::removeExpiredProjectiles() {
}

void ProjectileManager::clearAllProjectiles() {
}

// Template-based callback registration for better type safety
template <typename... Args>
void ProjectileManager::registerCallback(int key, void (ProjectileManager::*function)(Args...),
                                         Args... args) {
    keyBindings.emplace(key, [this, function, args...]() { (this->*function)(args...); });
}

// Specialized versions for common use cases
void ProjectileManager::registerCallback(int key, void (ProjectileManager::*function)()) {
    keyBindings.emplace(key, [this, function]() { (this->*function)(); });
}

void ProjectileManager::registerFireCallback(int key) {
    keyBindings.emplace(key, [this]() { this->fireProjectile(); });
}

void ProjectileManager::setDefaultKeyBindings() {
    registerFireCallback(GLFW_MOUSE_BUTTON_LEFT);

    // Example of additional bindings you might want
    registerCallback(GLFW_KEY_Q, &ProjectileManager::switchProjectileType);
}

void ProjectileManager::mouseButtonEvent(int button, int action, int mods) {
    // Only trigger on press events, not release
    if (action != GLFW_PRESS) return;

    // Menu clicks (e.g. "Enter Game") can reach this callback before ImGui recomputes
    // WantCaptureMouse for the frame, so gate on GameState directly rather than relying on that flag alone.
    if (Game::state != GameState::RUNNING) return;

    if (ImGui::GetIO().WantCaptureMouse) return;

    auto it = keyBindings.find(button);
    if (it != keyBindings.end()) {
        it->second();  // Execute the callback
    } else if (isLoggingEnabled) {
        std::cout << "No callback registered for mouse button: " << button << std::endl;
    }
}

void ProjectileManager::setLogging(bool enabled) {
    isLoggingEnabled = enabled;
}

std::vector<std::string> ProjectileManager::getAvailableProjectileTypes() const {
    std::vector<std::string> types;
    types.reserve(projectileTemplates.size());

    for (const auto& projectile : projectileTemplates) {
        types.push_back(projectile.UID);
    }

    return types;
}

void ProjectileManager::onKeyEvent(int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    auto it = keyBindings.find(key);
    if (it != keyBindings.end()) {
        it->second();  // Execute the callback
    } else if (isLoggingEnabled) {
        std::cout << "No callback registered for key: " << key << std::endl;
    }
}

void ProjectileManager::onContinuousInput(double deltaTime, const KeyHandling& keyHandler) {
}
