#pragma once

#include "CollisionShapeGrid.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entity/fwd.hpp"
#include "rendering/Model.hpp"

class StaticTriangleSource {
   public:
    CollisionShapeGrid collisionGrid;
    std::unique_ptr<Model> model;

    StaticTriangleSource(std::unique_ptr<Model> model, std::vector<uint32_t> triangles, int minX,
                         int maxX, int minZ, int maxZ);
    ~StaticTriangleSource() = default;

    Model* getModel() { return model.get(); }

    int minX, maxX, minZ, maxZ;
    uint32_t currentSeed;
};