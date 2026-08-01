#include "CollisionManager.hpp"

#include "collision/CollisionResult.hpp"
#include "collision/ContactManifold.hpp"
#include "collision/shapes/OBB.hpp"
#include "collision/shapes/Sphere.hpp"
#include "collision/shapes/Triangle.hpp"
#include "components/collision/ShapeData.hpp"
#include "components/physics/JustCreated.hpp"
#include "components/physics/PhysicsComponents.hpp"
#include "core/CommandBuffer.hpp"
#include "core/Core.hpp"
#include "core/Game.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "environment/ChunkManager.hpp"
#include "environment/GridHelper.hpp"
#include "environment/TerrainConfig.hpp"
#include "environment/TriangleManager.hpp"

CandidateSet CollisionManager::getCandidates(const glm::vec3& position) {
    CandidateSet candidates;

    CollisionShapeGrid* terrain = Chunks.getTerrainAt(position);
    if (!terrain) return candidates;  // chunk at this position hasn't streamed in yet

    candidates.triangleCandidates = terrain->getAdjacentTriangles(position);
    candidates.dynamicCandidates = terrain->getAdjacentObjects(position);

    return candidates;
}

std::vector<ContactManifold> CollisionManager::checkIntersectionsAtPoint(
    const glm::vec3& position, const ShapeData& incidentVolume) {
    CandidateSet candidates = getCandidates(position);
    return checkIntersectionsFromCandidates(candidates.dynamicCandidates,
                                            candidates.triangleCandidates, incidentVolume);
}

std::vector<ContactManifold> CollisionManager::checkIntersectionsFromCandidates(
    const std::vector<entt::entity>& dynamicCandidates,
    const std::vector<uint32_t>& triangleCandidates, const ShapeData& incidentVolume) {
    std::vector<ContactManifold> results;

    for (auto candidate : dynamicCandidates) {
        ShapeData& shapeData = Registry.get<ShapeData>(candidate);
        CollisionResult result = checkIntersection(shapeData, incidentVolume);
        if (result.collided) {
            results.push_back(populateManifold(incidentVolume, shapeData, result));
        }
    }

    for (auto triangleId : triangleCandidates) {
        const TriangleData& triangleData = Triangles.get(triangleId);
        CollisionResult result = checkIntersection(incidentVolume, triangleData);
        if (result.collided) {
            results.push_back(populateManifold(incidentVolume, triangleData, result));
        }
    }

    return results;
}

void CollisionManager::processHeightMapCollisions(std::vector<entt::entity> dynamicObjects,
                                                  float duration) {
    for (auto dynamicEntity : dynamicObjects) {
        Position position = Registry.get<Position>(dynamicEntity);
        ShapeData& dynamicShape = Registry.get<ShapeData>(dynamicEntity);

        if (std::holds_alternative<TriangleData>(dynamicShape.data)) continue;

        glm::vec3 currentCenter =
            std::visit([](const auto& data) { return data.currentCenter; }, dynamicShape.data);
        float footprintX =
            std::visit(ExtentAlongDirectionVisitor{glm::vec3(1.0f, 0.0f, 0.0f)}, dynamicShape.data);
        float footprintZ =
            std::visit(ExtentAlongDirectionVisitor{glm::vec3(0.0f, 0.0f, 1.0f)}, dynamicShape.data);

        glm::ivec2 minChunk =
            GridHelper::worldToChunkIndex(currentCenter - glm::vec3(footprintX, 0.0f, footprintZ));
        glm::ivec2 maxChunk =
            GridHelper::worldToChunkIndex(currentCenter + glm::vec3(footprintX, 0.0f, footprintZ));

        std::vector<uint32_t> triangleCandidates;
        for (int cx = minChunk.x; cx <= maxChunk.x; ++cx) {
            for (int cz = minChunk.y; cz <= maxChunk.y; ++cz) {
                auto it = Chunks.chunkCollisionGrid.find(glm::ivec2(cx, cz));
                if (it == Chunks.chunkCollisionGrid.end()) continue;

                std::vector<uint32_t> local = it->second->getAdjacentTriangles(dynamicEntity);
                triangleCandidates.insert(triangleCandidates.end(), local.begin(), local.end());
            }
        }
        std::sort(triangleCandidates.begin(), triangleCandidates.end());
        triangleCandidates.erase(std::unique(triangleCandidates.begin(), triangleCandidates.end()),
                                 triangleCandidates.end());

        bool touchingTerrain = false;
        for (auto triangleId : triangleCandidates) {
            const TriangleData& triangleData = Triangles.get(triangleId);

            if (checkIntersection(dynamicShape, triangleData).collided) {
                touchingTerrain = true;
                break;
            }
        }

        // Height/normal come from the owning chunk's baked heightmap; skip the entity entirely
        // when its chunk isn't loaded, since there is no terrain to collide with yet.
        float height;
        if (!Chunks.sampleBakedHeight(currentCenter.x, currentCenter.z, height)) continue;

        glm::vec3 normal = Chunks.sampleBakedNormal(currentCenter.x, currentCenter.z);

        glm::vec3 terrainPoint(currentCenter.x, height, currentCenter.z);

        float extentAlongNormal =
            std::visit(ExtentAlongDirectionVisitor{normal}, dynamicShape.data);

        glm::vec3 contactPosition = currentCenter - normal * extentAlongNormal;

        glm::vec3 distanceToCenter = currentCenter - terrainPoint;

        float normalDistance = glm::dot(distanceToCenter, normal);

        float penetrationDepth = extentAlongNormal - normalDistance;

        if (touchingTerrain) {
            glm::vec3 velBefore = Registry.get<Velocity>(dynamicEntity).linear;

            contactResolver.resolveHeightMapCollision(contactPosition, normal, penetrationDepth,
                                                      duration, dynamicEntity);
        }
    }
}

std::vector<ContactManifold> CollisionManager::checkDynamicIntersections(
    std::vector<entt::entity> dynamicObjects) {
    std::vector<ContactManifold> results;

    for (auto entityA : dynamicObjects) {
        for (auto entityB : dynamicObjects) {
            if (entityA == entityB) continue;

            ShapeData& shapeA = Registry.get<ShapeData>(entityA);
            ShapeData& shapeB = Registry.get<ShapeData>(entityB);

            CollisionResult result = checkIntersection(shapeA, shapeB);
            if (result.collided) {
                results.push_back(populateManifold(shapeA, shapeB, result));
            }
        }
    }

    return results;
}

void CollisionManager::processCollisions(float duration) {
    auto view = Registry.view<Velocity, ShapeData>();

    std::vector<entt::entity> dynamicObjects;

    for (auto entity : view) {
        if (Registry.all_of<JustCreated>(entity)) continue;
        dynamicObjects.push_back(entity);
    }

    processHeightMapCollisions(dynamicObjects, duration);

    std::vector<ContactManifold> dynamicResults = checkDynamicIntersections(dynamicObjects);

    for (auto& manifold : dynamicResults) {
        contactResolver.resolveCollisionImpulse(manifold, duration, manifold.a, manifold.b);
    }
}

CollisionResult CollisionManager::checkIntersection(const ShapeData& objectA,
                                                    const ShapeData& objectB) {
    return std::visit(CollisionVisitor{}, objectA.data, objectB.data);
}

CollisionResult CollisionManager::checkIntersection(const ShapeData& objectA,
                                                    const TriangleData& triangleB) {
    return std::visit(CollisionVisitor{}, objectA.data,
                      std::variant<TriangleData, SphereData, OBBData>(triangleB));
}

ContactManifold CollisionManager::populateManifold(const ShapeData& objectA,
                                                   const ShapeData& objectB,
                                                   CollisionResult result) {
    ContactManifold manifold(objectA.owner, objectB.owner);

    std::visit(ManifoldVisitor{manifold, result}, objectA.data, objectB.data);

    return manifold;
}

// Triangles have no owning entity - only objectA contributes an owner to the manifold.
ContactManifold CollisionManager::populateManifold(const ShapeData& objectA,
                                                   const TriangleData& triangleB,
                                                   CollisionResult result) {
    ContactManifold manifold(objectA.owner, entt::null);

    std::visit(ManifoldVisitor{manifold, result}, objectA.data,
               std::variant<TriangleData, SphereData, OBBData>(triangleB));

    return manifold;
}