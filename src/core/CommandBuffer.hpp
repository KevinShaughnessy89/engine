#pragma once
#include <mutex>
#include <tracy/Tracy.hpp>
#include <vector>

#include "core/CommandTypes.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "physics/ProjectileManager.hpp"

/*
 * CommandBuffer - gathers intermediate ECS data (as command structs) from any
 * thread, then turns them into entt entities on the main thread. Producers
 * push from worker threads; the main thread drains via processEntityCreation(),
 * so all registry mutation stays on the main thread.
 */
class CommandBuffer {
   public:
    void push(EngineCommand command) {
        std::lock_guard lock(mutex);
        commands.emplace_back(std::move(command));
    }

    void processEntityCreation() {
        ZoneScoped;
        std::vector<EngineCommand> pending;
        {
            std::lock_guard lock(mutex);
            pending.swap(commands);
        }
        ZoneValue(pending.size());

        for (auto& command : pending) {
            std::visit([](auto& cmd) { execute(cmd); }, command);
        }
    }

   private:
    static void execute(const ProjectileSpawnCommand& command) {
        Projectiles.createProjectile(command);
    }

    std::vector<EngineCommand> commands;
    std::mutex mutex;
};
