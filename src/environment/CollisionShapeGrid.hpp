#pragma once
#include "TerrainConfig.hpp"
#include "collision/CollisionAliases.hpp"
#include "components/collision/ShapeData.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entity/fwd.hpp"
#include "entt-main/src/entt/entt.hpp"

class CollisionObject;

struct GridCell {
    glm::vec2 minCoord;
    glm::vec2 maxCoord;
    std::vector<entt::entity> objects;  // SphereData / OBBData
    std::vector<uint32_t> triangles;    // TriangleData
};

class CollisionShapeGrid {
   public:
    std::vector<std::vector<GridCell>> grid;
    glm::vec2 gridMin, gridMax;
    int numCellsX, numCellsZ;
    int resolution = TerrainConfig::gridResolution;

    CollisionShapeGrid() = default;

    CollisionShapeGrid(glm::vec2 gridMin, glm::vec2 gridMax, std::vector<entt::entity> objects);
    CollisionShapeGrid(glm::vec2 gridMin, glm::vec2 gridMax, std::vector<uint32_t> triangles);
    // Builds an empty grid sized for numShapes objects to be inserted later via insertObject(),
    // reserving each cell's capacity up front instead of growing it one push_back at a time.
    CollisionShapeGrid(glm::vec2 gridMin, glm::vec2 gridMax, int numShapes);

    void insertObject(entt::entity object);
    int insertObject(uint32_t triangleId);

    std::vector<entt::entity> getObjectsInArea(const glm::vec2& gridMin, const glm::vec2& gridMax);
    std::vector<uint32_t> getTrianglesInArea(const glm::vec2& gridMin, const glm::vec2& gridMax);

    std::vector<entt::entity> getGridObjects();
    std::vector<uint32_t> getGridTriangles();

    std::vector<entt::entity> getAdjacentObjects(entt::entity object);
    std::vector<uint32_t> getAdjacentObjects(uint32_t triangleId);

    // Static terrain triangles are never queried directly (they don't move), but a dynamic
    // entity needs to find the triangles adjacent to *it* - hence entt::entity in, uint32_t
    // triangle ids out.
    std::vector<uint32_t> getAdjacentTriangles(entt::entity object);

    std::vector<entt::entity> getAdjacentObjects(const glm::vec3& worldPos);
    std::vector<uint32_t> getAdjacentTriangles(const glm::vec3& worldPos);

   private:
    void initCells();
    void getCellRange(const glm::vec3& min, const glm::vec3& max, int& minGridX, int& minGridZ,
                      int& maxGridX, int& maxGridZ) const;
};
