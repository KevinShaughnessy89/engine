#include "StaticTriangleSource.hpp"

#include "core/PrecompiledHeader.hpp"

StaticTriangleSource::StaticTriangleSource(std::unique_ptr<Model> model,
                                           std::vector<uint32_t> triangles, int minX, int maxX,
                                           int minZ, int maxZ)
    : minX(minX), maxX(maxX), minZ(minZ), maxZ(maxZ) {
    this->model = std::move(model);
    this->collisionGrid =
        CollisionShapeGrid(glm::vec2(minX, minZ), glm::vec2(maxX, maxZ), triangles);
}