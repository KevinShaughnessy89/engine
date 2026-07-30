#pragma once
#include "collision/shapes/OBB.hpp"
#include "collision/shapes/Sphere.hpp"
#include "components/collision/ShapeData.hpp"
#include "components/physics/Orientation.hpp"
#include "components/physics/Position.hpp"
#include "components/physics/Velocity.hpp"
#include "components/rendering/Renderable.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"

/*
 * ShapeUpdater - syncs the collision shapes of active dynamic objects
 * (Velocity + ShapeData + Renderable) to their physics transforms
 * (Position + Orientation, the source of truth). Runs on the fixed-rate
 * clock thread, sequentially between Physics.runPhysics() and
 * Collision.processCollisions(), so collision always sees shapes that match
 * the transforms physics just wrote and nothing mutates them mid-solve.
 */

class ShapeUpdater {
   public:
    void update() {
        auto view = Registry.view<Velocity, ShapeData, Renderable, Position, Orientation>();

        for (auto entity : view) {
            auto& shapeData = view.get<ShapeData>(entity);
            const auto& position = view.get<Position>(entity);
            const auto& orientation = view.get<Orientation>(entity);
            if (shapeData.isStatic) {
                continue;
            }
            std::visit(
                [&position, &orientation](auto& shape) {
                    using T = std::decay_t<decltype(shape)>;
                    if constexpr (std::is_same_v<T, SphereData>) {
                        Sphere::update(shape, position, orientation);
                    } else if constexpr (std::is_same_v<T, OBBData>) {
                        OBB::update(shape, position, orientation);
                    }
                    // TriangleData: static geometry, nothing to update
                },
                shapeData.data);
        }
    }
};
