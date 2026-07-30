#pragma once

#include "CollisionResult.hpp"
#include "ContactPoint.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"

class ContactManifold;

class ContactResolver {
   public:
    ContactResolver();
    std::vector<float> gompertzNodes;
    std::vector<glm::vec3> rendering;
    float averageTime;
    float referenceAverageTime;
    float cumulativeDrift;
    static void resolveCollisionImpulse(ContactManifold& manifold, const float duration,
                                        entt::entity object1, entt::entity object2);
    static void resolveHeightMapCollision(glm::vec3 position, const glm::vec3& normal,
                                          float penetrationDepth, const float duration,
                                          entt::entity object);
    // FeatureID* getAABBFeatureID(const glm::vec3& pointCoordinates);
    glm::vec3 calculateMomentArm(const ContactManifold& manifold, entt::entity object);
    glm::vec3 calculateSingleContactMomentArm(const ContactManifold& manifold, entt::entity object);
    glm::vec3 calculateTwoContactMomentArm(const ContactManifold& manifold, entt::entity object);
    glm::vec3 calculateThreeContactMomentArm(const ContactManifold& manifold, entt::entity object);
    glm::vec3 calculateMultiContactMomentArm(const ContactManifold& manifold, entt::entity object);
    bool isCornerInContact(const glm::vec3& corner, const ContactManifold& manifold);
    glm::vec3 findNearestContactPoint(const glm::vec3& point, const ContactManifold& manifold);
};