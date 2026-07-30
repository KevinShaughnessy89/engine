#pragma once
#include "core/CommandTypes.hpp"
#include "core/Core.hpp"
#include "core/KeyHandling.hpp"
#include "core/KeyObserver.hpp"
#include "core/MouseObserver.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "physics/ProjectileManagerStructs.hpp"

class Model;

class ProjectileManager : public MouseObserver, public KeyObserver {
   private:
    std::vector<ProjectileAsset> projectileTemplates;  // Templates for creating new projectiles
    std::vector<entt::entity> projectileEntityRegistry;
    // Models are owned by the ModelLoader registry; this just tracks which entity each
    // projectile's Renderable points at.
    std::vector<std::pair<entt::entity, Model*>> projectileModels;
    std::unordered_map<int, std::function<void()>> keyBindings;

    void onKeyEvent(int key, int scancode, int action, int mods) override;
    void onContinuousInput(double deltaTime, const KeyHandling& keyHandler) override;

    bool isLoggingEnabled = false;

    void removeExpiredProjectiles();

   public:
    ProjectileManager();
    ~ProjectileManager();

    std::vector<std::string> availableProjectileTypes = {"box", "sphere"};
    std::string currentProjectileType = "box";
    int currentProjectileTypeIndex = 0;

    void switchProjectileType();

    // Delete copy operations to prevent issues with unique_ptr
    ProjectileManager(const ProjectileManager&) = delete;
    ProjectileManager& operator=(const ProjectileManager&) = delete;

    // Move operations are fine
    ProjectileManager(ProjectileManager&&) = default;
    ProjectileManager& operator=(ProjectileManager&&) = default;

    void init();

    // Core projectile management
    void addProjectileAsset(const ProjectileAsset& projectile);
    void fireProjectile();
    void createProjectile(const ProjectileSpawnCommand& command);  // main thread only
    void clearAllProjectiles();

    // Callback registration system (improved interface)
    void registerCallback(int key, void (ProjectileManager::*function)());
    void registerFireCallback(int key);
    void setDefaultKeyBindings();

    // Template version for more complex callbacks (optional advanced usage)
    template <typename... Args>
    void registerCallback(int key, void (ProjectileManager::*function)(Args...), Args... args);

    // Input handling
    void mouseButtonEvent(int button, int action, int mods) override;

    // Utility methods
    void setLogging(bool enabled);
    std::vector<std::string> getAvailableProjectileTypes() const;
};
