#include "CollisionShapeGrid.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>

#include "collision/CollisionVisitor.hpp"
#include "collision/shapes/Triangle.hpp"
#include "components/collision/ShapeData.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entity/fwd.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "environment/EnvironmentState.hpp"
#include "environment/TriangleManager.hpp"

CollisionShapeGrid::CollisionShapeGrid(glm::vec2 gridMin, glm::vec2 gridMax,
                                       std::vector<entt::entity> objects)
    : gridMin(gridMin), gridMax(gridMax) {
    initCells();

    for (auto& object : objects) {
        insertObject(object);
    }
}

CollisionShapeGrid::CollisionShapeGrid(glm::vec2 gridMin, glm::vec2 gridMax,
                                       std::vector<uint32_t> triangles)
    : gridMin(gridMin), gridMax(gridMax) {
    initCells();

    long long totalPushbacks = 0;
    for (auto triangleId : triangles) {
        totalPushbacks += insertObject(triangleId);
    }

    std::cout << "[PROFILE] CollisionShapeGrid insertObjects pushbacks: " << totalPushbacks
              << std::endl;
}

CollisionShapeGrid::CollisionShapeGrid(glm::vec2 gridMin, glm::vec2 gridMax, int numShapes)
    : gridMin(gridMin), gridMax(gridMax) {
    initCells();

    int cellCount = numCellsX * numCellsZ;
    int reservePerCell = cellCount > 0 ? (numShapes / cellCount) + 1 : numShapes;

    for (auto& row : grid) {
        for (auto& cell : row) {
            cell.objects.reserve(reservePerCell);
        }
    }
}

void CollisionShapeGrid::initCells() {
    float width = gridMax.x - gridMin.x;
    float depth = gridMax.y - gridMin.y;
    numCellsX = resolution;
    numCellsZ = resolution;

    float cellWidth = width / numCellsX;
    float cellDepth = depth / numCellsZ;

    grid.clear();
    grid.resize(numCellsX);

    for (auto& row : grid) {
        row.resize(numCellsZ);
    }

    for (int x = 0; x < numCellsX; ++x) {
        for (int z = 0; z < numCellsZ; ++z) {
            grid[x][z].maxCoord =
                gridMin + glm::vec2(x * cellWidth + cellWidth, z * cellDepth + cellDepth);
            grid[x][z].minCoord = gridMin + glm::vec2(x * cellWidth, z * cellDepth);
        }
    }
}

void CollisionShapeGrid::getCellRange(const glm::vec3& min, const glm::vec3& max, int& minGridX,
                                      int& minGridZ, int& maxGridX, int& maxGridZ) const {
    float normalizedMinX = (min.x - gridMin.x) / (gridMax.x - gridMin.x);
    float normalizedMinZ = (min.z - gridMin.y) / (gridMax.y - gridMin.y);
    float normalizedMaxX = (max.x - gridMin.x) / (gridMax.x - gridMin.x);
    float normalizedMaxZ = (max.z - gridMin.y) / (gridMax.y - gridMin.y);

    minGridX = std::min(static_cast<int>(normalizedMinX * numCellsX), numCellsX - 1);
    minGridZ = std::min(static_cast<int>(normalizedMinZ * numCellsZ), numCellsZ - 1);
    maxGridX = std::min(static_cast<int>(normalizedMaxX * numCellsX), numCellsX - 1);
    maxGridZ = std::min(static_cast<int>(normalizedMaxZ * numCellsZ), numCellsZ - 1);

    minGridX = std::max(0, std::min(minGridX, numCellsX - 1));
    minGridZ = std::max(0, std::min(minGridZ, numCellsZ - 1));
    maxGridX = std::max(0, std::min(maxGridX, numCellsX - 1));
    maxGridZ = std::max(0, std::min(maxGridZ, numCellsZ - 1));
}

void CollisionShapeGrid::insertObject(entt::entity object) {
    ShapeData& shapeData = Registry.get<ShapeData>(object);
    std::visit(
        [&](auto& data) {
            glm::vec3 halfExtent = (data.max - data.min) * 0.5f;
            glm::vec3 worldMin = data.currentCenter - halfExtent;
            glm::vec3 worldMax = data.currentCenter + halfExtent;

            int minGridX, minGridZ, maxGridX, maxGridZ;
            getCellRange(worldMin, worldMax, minGridX, minGridZ, maxGridX, maxGridZ);

            for (int x = minGridX; x <= maxGridX; ++x) {
                for (int z = minGridZ; z <= maxGridZ; ++z) {
                    grid[x][z].objects.push_back(object);
                }
            }
        },
        shapeData.data);
}

int CollisionShapeGrid::insertObject(uint32_t triangleId) {
    const TriangleData& triangleData = Triangles.get(triangleId);

    int minGridX, minGridZ, maxGridX, maxGridZ;
    getCellRange(triangleData.min, triangleData.max, minGridX, minGridZ, maxGridX, maxGridZ);

    int pushbacks = 0;
    for (int x = minGridX; x <= maxGridX; ++x) {
        for (int z = minGridZ; z <= maxGridZ; ++z) {
            grid[x][z].triangles.push_back(triangleId);
            pushbacks++;
        }
    }
    return pushbacks;
}

// Shared by every "adjacent to a center point with some extent" query below - the
// entt::entity/uint32_t overloads just differ in how they resolve center/extent and which
// cell vector they read from.
static void computeCenterCellRange(const glm::vec2& gridMin, const glm::vec2& gridMax,
                                   int numCellsX, int numCellsZ, const glm::vec3& currentCenter,
                                   float extentX, float extentZ, int& centerGridX, int& centerGridZ,
                                   int& gridsToTraverseX, int& gridsToTraverseZ) {
    float normalizedX = (currentCenter.x - gridMin.x) / (gridMax.x - gridMin.x);
    float normalizedZ = (currentCenter.z - gridMin.y) / (gridMax.y - gridMin.y);

    gridsToTraverseX = extentX / ((gridMax.x - gridMin.x) / numCellsX) + 1;
    gridsToTraverseZ = extentZ / ((gridMax.y - gridMin.y) / numCellsZ) + 1;

    centerGridX = std::min(static_cast<int>(normalizedX * numCellsX), numCellsX - 1);
    centerGridZ = std::min(static_cast<int>(normalizedZ * numCellsZ), numCellsZ - 1);

    centerGridX = std::max(0, centerGridX);
    centerGridZ = std::max(0, centerGridZ);
}

std::vector<entt::entity> CollisionShapeGrid::getAdjacentObjects(entt::entity object) {
    std::vector<entt::entity> result;

    ShapeData& shapeData = Registry.get<ShapeData>(object);

    glm::vec3 currentCenter =
        std::visit([](const auto& data) { return data.currentCenter; }, shapeData.data);

    float extentX =
        std::visit(ExtentAlongDirectionVisitor{glm::vec3(1.0f, 0.0f, 0.0f)}, shapeData.data);
    float extentZ =
        std::visit(ExtentAlongDirectionVisitor{glm::vec3(0.0f, 0.0f, 1.0f)}, shapeData.data);

    int centerGridX, centerGridZ, gridsToTraverseX, gridsToTraverseZ;
    computeCenterCellRange(gridMin, gridMax, numCellsX, numCellsZ, currentCenter, extentX, extentZ,
                           centerGridX, centerGridZ, gridsToTraverseX, gridsToTraverseZ);

    for (int dx = -gridsToTraverseX; dx <= gridsToTraverseX; ++dx) {
        for (int dz = -gridsToTraverseZ; dz <= gridsToTraverseZ; ++dz) {
            int gridX = centerGridX + dx;
            int gridZ = centerGridZ + dz;

            if (gridX < 0 || gridX >= numCellsX || gridZ < 0 || gridZ >= numCellsZ) {
                continue;
            }

            for (auto& object : grid[gridX][gridZ].objects) {
                result.push_back(object);
            }
        }
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

std::vector<uint32_t> CollisionShapeGrid::getAdjacentObjects(uint32_t triangleId) {
    std::vector<uint32_t> result;

    const TriangleData& triangleData = Triangles.get(triangleId);

    glm::vec3 currentCenter = triangleData.currentCenter;

    float extentX = Triangle::getExtentAlongDirection(triangleData, glm::vec3(1.0f, 0.0f, 0.0f));
    float extentZ = Triangle::getExtentAlongDirection(triangleData, glm::vec3(0.0f, 0.0f, 1.0f));

    int centerGridX, centerGridZ, gridsToTraverseX, gridsToTraverseZ;
    computeCenterCellRange(gridMin, gridMax, numCellsX, numCellsZ, currentCenter, extentX, extentZ,
                           centerGridX, centerGridZ, gridsToTraverseX, gridsToTraverseZ);

    for (int dx = -gridsToTraverseX; dx <= gridsToTraverseX; ++dx) {
        for (int dz = -gridsToTraverseZ; dz <= gridsToTraverseZ; ++dz) {
            int gridX = centerGridX + dx;
            int gridZ = centerGridZ + dz;

            if (gridX < 0 || gridX >= numCellsX || gridZ < 0 || gridZ >= numCellsZ) {
                continue;
            }

            for (auto& id : grid[gridX][gridZ].triangles) {
                result.push_back(id);
            }
        }
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

std::vector<uint32_t> CollisionShapeGrid::getAdjacentTriangles(entt::entity object) {
    std::vector<uint32_t> result;

    ShapeData& shapeData = Registry.get<ShapeData>(object);

    glm::vec3 currentCenter =
        std::visit([](const auto& data) { return data.currentCenter; }, shapeData.data);

    float extentX =
        std::visit(ExtentAlongDirectionVisitor{glm::vec3(1.0f, 0.0f, 0.0f)}, shapeData.data);
    float extentZ =
        std::visit(ExtentAlongDirectionVisitor{glm::vec3(0.0f, 0.0f, 1.0f)}, shapeData.data);

    int centerGridX, centerGridZ, gridsToTraverseX, gridsToTraverseZ;
    computeCenterCellRange(gridMin, gridMax, numCellsX, numCellsZ, currentCenter, extentX, extentZ,
                           centerGridX, centerGridZ, gridsToTraverseX, gridsToTraverseZ);

    for (int dx = -gridsToTraverseX; dx <= gridsToTraverseX; ++dx) {
        for (int dz = -gridsToTraverseZ; dz <= gridsToTraverseZ; ++dz) {
            int gridX = centerGridX + dx;
            int gridZ = centerGridZ + dz;

            if (gridX < 0 || gridX >= numCellsX || gridZ < 0 || gridZ >= numCellsZ) {
                continue;
            }

            for (auto& id : grid[gridX][gridZ].triangles) {
                result.push_back(id);
            }
        }
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

std::vector<entt::entity> CollisionShapeGrid::getAdjacentObjects(const glm::vec3& worldPos) {
    std::vector<entt::entity> result;

    float normalizedX = (worldPos.x - gridMin.x) / (gridMax.x - gridMin.x);
    float normalizedZ = (worldPos.z - gridMin.y) / (gridMax.y - gridMin.y);

    int centerGridX =
        std::min(static_cast<int>(normalizedX * numCellsX), static_cast<int>(numCellsX) - 1);
    int centerGridZ =
        std::min(static_cast<int>(normalizedZ * numCellsZ), static_cast<int>(numCellsZ) - 1);

    centerGridX = std::max(0, centerGridX);
    centerGridZ = std::max(0, centerGridZ);

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            int gridX = centerGridX + dx;
            int gridZ = centerGridZ + dz;

            if (gridX < 0 || gridX >= numCellsX || gridZ < 0 || gridZ >= numCellsZ) {
                continue;
            }

            for (auto& object : grid[gridX][gridZ].objects) {
                result.push_back(object);
            }
        }
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

std::vector<uint32_t> CollisionShapeGrid::getAdjacentTriangles(const glm::vec3& worldPos) {
    std::vector<uint32_t> result;

    float normalizedX = (worldPos.x - gridMin.x) / (gridMax.x - gridMin.x);
    float normalizedZ = (worldPos.z - gridMin.y) / (gridMax.y - gridMin.y);

    int centerGridX =
        std::min(static_cast<int>(normalizedX * numCellsX), static_cast<int>(numCellsX) - 1);
    int centerGridZ =
        std::min(static_cast<int>(normalizedZ * numCellsZ), static_cast<int>(numCellsZ) - 1);

    centerGridX = std::max(0, centerGridX);
    centerGridZ = std::max(0, centerGridZ);

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            int gridX = centerGridX + dx;
            int gridZ = centerGridZ + dz;

            if (gridX < 0 || gridX >= numCellsX || gridZ < 0 || gridZ >= numCellsZ) {
                continue;
            }

            for (auto& id : grid[gridX][gridZ].triangles) {
                result.push_back(id);
            }
        }
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

std::vector<entt::entity> CollisionShapeGrid::getObjectsInArea(const glm::vec2& lookMin,
                                                               const glm::vec2& lookMax) {
    std::vector<entt::entity> result;
    std::unordered_set<entt::entity> uniqueObjects;

    int minGridX, minGridZ, maxGridX, maxGridZ;
    getCellRange(glm::vec3(lookMin.x, 0.0f, lookMin.y), glm::vec3(lookMax.x, 0.0f, lookMax.y),
                 minGridX, minGridZ, maxGridX, maxGridZ);

    for (int x = minGridX; x <= maxGridX; ++x) {
        for (int y = minGridZ; y <= maxGridZ; ++y) {
            for (auto object : grid[x][y].objects) {
                uniqueObjects.insert(object);
            }
        }
    }

    result.reserve(uniqueObjects.size());
    for (auto objectPtr : uniqueObjects) {
        result.push_back(objectPtr);
    }

    return result;
}

std::vector<uint32_t> CollisionShapeGrid::getTrianglesInArea(const glm::vec2& lookMin,
                                                             const glm::vec2& lookMax) {
    std::vector<uint32_t> result;
    std::unordered_set<uint32_t> uniqueTriangles;

    int minGridX, minGridZ, maxGridX, maxGridZ;
    getCellRange(glm::vec3(lookMin.x, 0.0f, lookMin.y), glm::vec3(lookMax.x, 0.0f, lookMax.y),
                 minGridX, minGridZ, maxGridX, maxGridZ);

    for (int x = minGridX; x <= maxGridX; ++x) {
        for (int y = minGridZ; y <= maxGridZ; ++y) {
            for (auto triangleId : grid[x][y].triangles) {
                uniqueTriangles.insert(triangleId);
            }
        }
    }

    result.reserve(uniqueTriangles.size());
    for (auto triangleId : uniqueTriangles) {
        result.push_back(triangleId);
    }

    return result;
}

std::vector<entt::entity> CollisionShapeGrid::getGridObjects() {
    std::vector<entt::entity> result;
    for (const auto& x : grid) {
        for (const auto& z : x) {
            result.insert(result.end(), z.objects.begin(), z.objects.end());
        }
    }
    return result;
}

std::vector<uint32_t> CollisionShapeGrid::getGridTriangles() {
    std::vector<uint32_t> result;
    for (const auto& x : grid) {
        for (const auto& z : x) {
            result.insert(result.end(), z.triangles.begin(), z.triangles.end());
        }
    }
    return result;
}
