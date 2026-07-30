#pragma once

#include "CollisionVisitor.hpp"
#include "collision/ContactManifold.hpp"
#include "collision/ContactResolver.hpp"
#include "components/collision/ShapeData.hpp"
#include "core/PrecompiledHeader.hpp"
#include "environment/TerrainConfig.hpp"

class PhysicsManager;
struct ContactPoint;
class OctTree;
// struct OctTree::Node;
struct CollisionResult;
class ContactManifold;

// Terrain triangles are addressed by uint32_t id (via the global Triangles manager) rather
// than entt::entity, so any candidate list drawn from CollisionShapeGrid has to carry the two
// kinds of id side by side instead of a single homogeneous vector.
struct CandidateSet {
    std::vector<entt::entity> dynamicCandidates;
    std::vector<uint32_t> triangleCandidates;
};

class CollisionManager {
   public:
    CollisionManager() {}
    ContactResolver contactResolver;
    std::vector<ContactManifold> checkDynamicIntersections(
        std::vector<entt::entity> dynamicObjects);
    void processHeightMapCollisions(std::vector<entt::entity> dynamicObjects, float duration);
    CollisionResult checkIntersection(const ShapeData& objectA, const ShapeData& objectB);
    CollisionResult checkIntersection(const ShapeData& objectA, const TriangleData& triangleB);
    ContactManifold populatManifold(CollisionResult result, const ShapeData& objectA,
                                    const ShapeData& objectB);
    CandidateSet getCandidates(const glm::vec3& position);
    std::vector<ContactManifold> checkIntersectionsAtPoint(const glm::vec3& position,
                                                           const ShapeData& incidentVolume);
    std::vector<ContactManifold> checkIntersectionsFromCandidates(
        const std::vector<entt::entity>& dynamicCandidates,
        const std::vector<uint32_t>& triangleCandidates, const ShapeData& incidentVolume);
    void processCollisions(float duration);
    ContactManifold populateManifold(const ShapeData& objectA, const ShapeData& objectB,
                                     CollisionResult result);
    ContactManifold populateManifold(const ShapeData& objectA, const TriangleData& triangleB,
                                     CollisionResult result);

    std::unordered_map<entt::entity, ContactManifold> thisFrameCollisions;
};