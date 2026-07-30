#pragma once
#include "ContactPoint.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"

class ContactManifold {
   public:
    std::vector<ContactPoint> contacts;
    int numContacts = 0;
    int maxContacts = 4;
    entt::entity a;
    entt::entity b;

    ContactManifold() = default;
    ContactManifold(entt::entity a, entt::entity b) : a(a), b(b) {}

    static constexpr float kDuplicateEpsilon = 1e-3f;

    void addContactPoint(const glm::vec3& position, const glm::vec3& normal,
                         float penetrationDepth) {
        if (numContacts >= maxContacts) {
            return;
        }

        for (const ContactPoint& existing : contacts) {
            if (glm::distance2(existing.position, position) <
                    kDuplicateEpsilon * kDuplicateEpsilon &&
                glm::distance2(existing.normal, normal) < kDuplicateEpsilon * kDuplicateEpsilon &&
                std::abs(existing.penetrationDepth - penetrationDepth) < kDuplicateEpsilon) {
                return;
            }
        }

        ContactPoint contact;
        contact.position = position;
        contact.normal = normal;
        contact.penetrationDepth = penetrationDepth;
        contacts.push_back(contact);
        numContacts++;

        std::sort(contacts.begin(), contacts.end(),
                  [](const ContactPoint& lhs, const ContactPoint& rhs) {
                      return lhs.penetrationDepth > rhs.penetrationDepth;
                  });
    }

    bool isSeperated = false;
};