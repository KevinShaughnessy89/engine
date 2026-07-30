#include "ContactResolver.hpp"

#include "collision/CollisionManager.hpp"
#include "collision/ContactManifold.hpp"
#include "components/physics/PhysicsComponents.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"

ContactResolver::ContactResolver() {
    averageTime = 0.15f;
}

bool ContactResolver::isCornerInContact(const glm::vec3& corner, const ContactManifold& manifold) {
    for (const auto& contact : manifold.contacts) {
        if (glm::distance(corner, contact.position) < Config::EPSILON) {
            return true;
        }
    }
    return false;
}

glm::vec3 ContactResolver::findNearestContactPoint(const glm::vec3& point,
                                                   const ContactManifold& manifold) {
    glm::vec3 nearestPoint = manifold.contacts[0].position;
    float minDistance = glm::distance(point, nearestPoint);

    for (size_t i = 1; i < manifold.contacts.size(); ++i) {
        float distance = glm::distance(point, manifold.contacts[i].position);
        if (distance < minDistance) {
            minDistance = distance;
            nearestPoint = manifold.contacts[i].position;
        }
    }

    return nearestPoint;
}

glm::vec3 getVelocityAtPoint(const glm::vec3& vel, const glm::vec3& angularVelocity,
                             const glm::vec3& pos, const glm::vec3& point) {
    return vel + glm::cross(vel, point - pos);
}

void ContactResolver::resolveCollisionImpulse(ContactManifold& manifold, const float duration,
                                              entt::entity object1, entt::entity object2) {
    if (manifold.contacts.empty()) {
        return;
    }

    // --- Retrieve components from ECS registry ---
    auto& pos1 = Registry.get<Position>(object1);
    auto& pos2 = Registry.get<Position>(object2);
    auto& vel1 = Registry.get<Velocity>(object1);
    auto& vel2 = Registry.get<Velocity>(object2);
    auto& invMass1 = Registry.get<InverseMass>(object1);
    auto& invMass2 = Registry.get<InverseMass>(object2);
    auto& inertia1 = Registry.get<Inertia>(object1);
    auto& inertia2 = Registry.get<Inertia>(object2);
    auto& orient1 = Registry.get<Orientation>(object1);
    auto& orient2 = Registry.get<Orientation>(object2);

    // Helper: replaces obj->getVelocityAtPoint(p)
    // = linear + cross(angular, r)  where r = p - position
    auto velocityAtPoint = [](const Velocity& vel, const Position& pos, const glm::vec3& point) {
        glm::vec3 r = point - pos.current;
        return vel.linear + glm::cross(vel.angular, r);
    };

    const float epsilon = 0.0f;
    const float minPenetrationDepth = 0.005f;
    const float frictionCoeff = 0.2f;
    const float velocityThreshold = 2.0f;
    const float impulseScale = 2.0f;
    const float beta = 0.25f;
    const float percent = 0.4f;
    const float slop = 0.01f;
    const float maxLinearCorrection = 100.0f;
    const float maxImpulse = 20.0f;
    const float angularImpulseScale = 200.f;
    const float maxImpulsePerMass = 50.0f;
    const float resterImpulseLimit = 0.5f;
    const float resterVelocityLimit = 5.f;
    const float globalVelocityDamping = 1.f;
    const float restingVelocityDamping = 0.97f;
    const float contactDamping = 0.98f;

    bool enableLogging = false;

    if (enableLogging) {
        int movingAwayCount = 0;
        int shallowCount = 0;
        float maxPenetration = 0.0f;

        for (const auto& contact : manifold.contacts) {
            glm::vec3 relativeVelocity = velocityAtPoint(vel2, pos2, contact.position) -
                                         velocityAtPoint(vel1, pos1, contact.position);
            float normalVelocity = glm::dot(relativeVelocity, contact.normal);
            if (normalVelocity > 0.0f) movingAwayCount++;
            if (contact.penetrationDepth <= slop) shallowCount++;
            maxPenetration = std::max(maxPenetration, contact.penetrationDepth);
        }
    }

    for (const auto& contact : manifold.contacts) {
        glm::vec3 ra = contact.position - pos1.current;
        glm::vec3 rb = contact.position - pos2.current;

        glm::vec3 relativeVelocity = velocityAtPoint(vel2, pos2, contact.position) -
                                     velocityAtPoint(vel1, pos1, contact.position);
        float normalVelocity = glm::dot(relativeVelocity, contact.normal);

        bool needsPositionCorrection = contact.penetrationDepth > slop;
        bool needsVelocityResolution = normalVelocity <= 0.0f;

        if (enableLogging) {
            std::cout << "Contact position: " << glm::to_string(contact.position) << "\n";
            std::cout << "Contact normal: " << glm::to_string(contact.normal) << "\n";
            std::cout << "Penetration depth: " << contact.penetrationDepth << "\n";
            std::cout << "Normal velocity: " << normalVelocity << "\n";
        }

        if (needsVelocityResolution) {
            glm::vec3 raCrossN = glm::cross(ra, contact.normal);
            glm::vec3 rbCrossN = glm::cross(rb, contact.normal);
            glm::vec3 Iinv_raCrossN = inertia1.inverseInertiaTensorWorld * raCrossN;
            glm::vec3 Iinv_rbCrossN = inertia2.inverseInertiaTensorWorld * rbCrossN;

            float impulseDenominator = invMass1.value + invMass2.value +
                                       glm::dot(raCrossN, Iinv_raCrossN) +
                                       glm::dot(rbCrossN, Iinv_rbCrossN);

            if (impulseDenominator < 1e-6f) continue;

            float baumgarte = (beta / duration) * std::max(contact.penetrationDepth - slop, 0.0f);

            float j = 0.0f;
            if (std::abs(normalVelocity) < velocityThreshold) {
                j = (-normalVelocity + baumgarte) / impulseDenominator;
            } else {
                j = (-(1.0f + epsilon) * normalVelocity + baumgarte) / impulseDenominator;
            }

            float combinedVelocity = (glm::length(vel1.linear) + glm::length(vel2.linear)) * 0.5f;
            bool isRestingContact = (std::abs(normalVelocity) < resterVelocityLimit &&
                                     combinedVelocity < resterVelocityLimit);

            if (isRestingContact) {
                float combinedInverseMass = invMass1.value + invMass2.value;
                j = std::min(j, resterImpulseLimit / combinedInverseMass);

                if (enableLogging) {
                    std::cout << "RESTING CONTACT - Limited impulse to: " << j << "\n";
                }
            } else {
                float combinedInverseMass = invMass1.value + invMass2.value;
                float massScaledLimit = maxImpulsePerMass / combinedInverseMass;
                j = std::min(j, massScaledLimit);
            }

            j = std::max(j, 0.0f);

            if (contact.penetrationDepth < slop * 2.0f &&
                j > resterImpulseLimit / (invMass1.value + invMass2.value)) {
                j = resterImpulseLimit / (invMass1.value + invMass2.value);
                if (enableLogging) {
                    std::cout << "SHALLOW PENETRATION - Reduced impulse\n";
                }
            }

            if (j > maxImpulse) j = maxImpulse;

            glm::vec3 normalImpulse = j * contact.normal;

            vel1.linear -= invMass1.value * normalImpulse;
            vel2.linear += invMass2.value * normalImpulse;

            vel1.angular -= inertia1.inverseInertiaTensorWorld * glm::cross(ra, normalImpulse) *
                            angularImpulseScale;
            vel2.angular += inertia2.inverseInertiaTensorWorld * glm::cross(rb, normalImpulse) *
                            angularImpulseScale;

            if (enableLogging) {
                std::cout << "Normal impulse scalar j: " << j << "\n";
                std::cout << "Normal impulse vector: " << glm::to_string(normalImpulse) << "\n";
                std::cout << "Obj1 velocity after impulse: " << glm::to_string(vel1.linear) << "\n";
                std::cout << "Obj2 velocity after impulse: " << glm::to_string(vel2.linear) << "\n";
                std::cout << "Obj1 angular velocity after impulse: " << glm::to_string(vel1.angular)
                          << "\n";
                std::cout << "Obj2 angular velocity after impulse: " << glm::to_string(vel2.angular)
                          << "\n";
            }

            if (isRestingContact) {
                vel1.linear *= restingVelocityDamping;
                vel1.angular *= restingVelocityDamping;
                vel2.linear *= restingVelocityDamping;
                vel2.angular *= restingVelocityDamping;
            } else {
                vel1.linear *= globalVelocityDamping;
                vel1.angular *= contactDamping;
                vel2.linear *= globalVelocityDamping;
                vel2.angular *= contactDamping;
            }

            // Recalculate relative velocity after normal impulse for friction
            relativeVelocity = velocityAtPoint(vel2, pos2, contact.position) -
                               velocityAtPoint(vel1, pos1, contact.position);
            normalVelocity = glm::dot(relativeVelocity, contact.normal);

            glm::vec3 tangentialVelocity = relativeVelocity - normalVelocity * contact.normal;
            float tangentialSpeed = glm::length(tangentialVelocity);

            if (tangentialSpeed > 1e-6f) {
                glm::vec3 tangent = tangentialVelocity / tangentialSpeed;

                glm::vec3 raCrossT = glm::cross(ra, tangent);
                glm::vec3 rbCrossT = glm::cross(rb, tangent);
                glm::vec3 Iinv_raCrossT = inertia1.inverseInertiaTensorWorld * raCrossT;
                glm::vec3 Iinv_rbCrossT = inertia2.inverseInertiaTensorWorld * rbCrossT;

                float frictionDenominator = invMass1.value + invMass2.value +
                                            glm::dot(raCrossT, Iinv_raCrossT) +
                                            glm::dot(rbCrossT, Iinv_rbCrossT);

                if (frictionDenominator > 1e-6f) {
                    float jt = -tangentialSpeed / frictionDenominator;

                    float maxFriction = frictionCoeff * j;
                    jt = glm::clamp(jt, -maxFriction, maxFriction);

                    glm::vec3 frictionImpulse = jt * tangent;

                    if (enableLogging) {
                        std::cout << "Friction tangent: " << glm::to_string(tangent) << "\n";
                        std::cout << "Friction scalar jt: " << jt << "\n";
                        std::cout << "Friction impulse: " << glm::to_string(frictionImpulse)
                                  << "\n";
                    }

                    vel1.linear -= invMass1.value * frictionImpulse;
                    vel2.linear += invMass2.value * frictionImpulse;

                    vel1.angular -=
                        inertia1.inverseInertiaTensorWorld * glm::cross(ra, frictionImpulse);
                    vel2.angular +=
                        inertia2.inverseInertiaTensorWorld * glm::cross(rb, frictionImpulse);
                }
            }
        }

        if (needsPositionCorrection) {
            float correctionMagnitude = percent * (contact.penetrationDepth - slop);
            glm::vec3 positionCorrection = correctionMagnitude * contact.normal;

            if (glm::length(positionCorrection) > maxLinearCorrection) {
                positionCorrection = glm::normalize(positionCorrection) * maxLinearCorrection;
            }

            if (enableLogging) {
                std::cout << "Position correction: " << glm::to_string(positionCorrection) << "\n";
            }

            float totalInverseMass = invMass1.value + invMass2.value;
            if (totalInverseMass > 0.0f) {
                glm::vec3 obj1Correction =
                    -(invMass1.value / totalInverseMass) * positionCorrection;
                glm::vec3 obj2Correction = (invMass2.value / totalInverseMass) * positionCorrection;

                if (invMass1.value > 0.0f) {
                    pos1.current += obj1Correction;
                    if (invMass1.value > 0.0f) {
                        pos1.current += obj1Correction;

                        glm::vec3 angularCorrection1 = inertia1.inverseInertiaTensorWorld *
                                                       glm::cross(ra, obj1Correction * 0.1f);
                        float angle1 = glm::length(angularCorrection1);
                        if (angle1 > 1e-6f) {
                            glm::vec3 axis1 = angularCorrection1 / angle1;
                            glm::mat3 rot1 = glm::mat3(glm::rotate(glm::mat4(1.0f), angle1, axis1));
                            orient1.rotationMatrix = rot1 * orient1.rotationMatrix;
                        }
                    }

                    if (invMass2.value > 0.0f) {
                        pos2.current += obj2Correction;

                        glm::vec3 angularCorrection2 = inertia2.inverseInertiaTensorWorld *
                                                       glm::cross(rb, obj2Correction * 0.1f);
                        float angle2 = glm::length(angularCorrection2);
                        if (angle2 > 1e-6f) {
                            glm::vec3 axis2 = angularCorrection2 / angle2;
                            glm::mat3 rot2 = glm::mat3(glm::rotate(glm::mat4(1.0f), angle2, axis2));
                            orient2.rotationMatrix = rot2 * orient2.rotationMatrix;
                        }
                    }
                }
            }
        }
    }
}

void ContactResolver::resolveHeightMapCollision(glm::vec3 position, const glm::vec3& normal,
                                                float penetrationDepth, float duration,
                                                entt::entity object) {
    // --- Retrieve components from ECS registry ---
    auto& pos = Registry.get<Position>(object);
    auto& vel = Registry.get<Velocity>(object);
    auto& invMass = Registry.get<InverseMass>(object);
    auto& inertia = Registry.get<Inertia>(object);
    auto& orient = Registry.get<Orientation>(object);
    auto& shapeData = Registry.get<ShapeData>(object);

    const float epsilon = 0.1f;
    const float frictionCoeff = 0.8;
    const float beta = 0.25f;
    const float percent = 0.4f;
    const float slop = 0.01f;
    const float maxLinearCorrection = 100.0f;

    glm::vec3 currentCenter =
        std::visit([](const auto& data) { return data.currentCenter; }, shapeData.data);

    float accumulatedImpulse = 0.f;

    bool loggingEnabled = false;

    if (loggingEnabled) {
        std::cout << "=== resolveHeightMapCollision START ===\n";
        std::cout << "duration: " << duration << "\n";
        std::cout << "Contact position: " << glm::to_string(position) << "\n";
        std::cout << "Contact normal: " << glm::to_string(normal) << "\n";
        std::cout << "Object position: " << glm::to_string(currentCenter) << "\n";
        std::cout << "Object linear velocity: " << glm::to_string(vel.linear) << "\n";
        std::cout << "Object angular velocity: " << glm::to_string(vel.angular) << "\n";
    }

    float baumgarte = (beta / duration) * std::max(penetrationDepth - slop, 0.0f);

    if (loggingEnabled) {
        std::cout << "Baumgarte stabilization term: " << baumgarte << "\n";
    }

    // Restitution target is fixed from the pre-solve approach velocity. If this were
    // recomputed from the live velocity every iteration (as -(1+epsilon)*normalVelocity),
    // later iterations would see the bounce velocity from iteration 0 as if it were a
    // fresh incoming approach, and solve it back toward zero - killing the bounce
    // regardless of epsilon.
    glm::vec3 rInitial = position - currentCenter;
    glm::vec3 initialVelocityAtContact = vel.linear + glm::cross(vel.angular, rInitial);
    float initialNormalVelocity = glm::dot(initialVelocityAtContact, normal);
    float restitutionBias =
        (initialNormalVelocity < 0.0f) ? -epsilon * initialNormalVelocity : 0.0f;

    int N_ITERATIONS = 4;
    for (int i = 0; i < N_ITERATIONS; i++) {
        if (loggingEnabled) {
            std::cout << "\n--- Iteration " << i << " ---\n";
        }

        glm::vec3 r = position - currentCenter;  // Vector from center of mass to contact point
        glm::vec3 velocityAtContact = vel.linear + glm::cross(vel.angular, r);
        float normalVelocity = glm::dot(velocityAtContact, normal);

        if (loggingEnabled) {
            std::cout << "r vector (contact - pos): " << glm::to_string(r) << "\n";
            std::cout << "Velocity at contact point: " << glm::to_string(velocityAtContact) << "\n";
            std::cout << "Normal velocity: " << normalVelocity << "\n";
        }

        glm::vec3 rCrossN = glm::cross(r, normal);
        glm::vec3 Iinv_rCrossN = inertia.inverseInertiaTensorWorld * rCrossN;
        float impulseDenominator = invMass.value + glm::dot(rCrossN, Iinv_rCrossN);

        if (loggingEnabled) {
            std::cout << "Impulse denominator: " << impulseDenominator << "\n";
        }

        if (impulseDenominator < 1e-6f) {
            if (loggingEnabled) {
                std::cout << "Impulse denominator too small, skipping\n";
            }
            continue;
        }

        float dLambda = (-normalVelocity + restitutionBias + baumgarte) / impulseDenominator;

        float newImpulse = std::max(accumulatedImpulse + dLambda, 0.0f);
        dLambda = newImpulse - accumulatedImpulse;
        accumulatedImpulse = newImpulse;

        glm::vec3 normalImpulse = dLambda * normal;

        if (loggingEnabled) {
            std::cout << "dLambda (impulse delta): " << dLambda << "\n";
            std::cout << "newImpulse (accumulated): " << newImpulse << "\n";
            std::cout << "Accumulated impulse: " << accumulatedImpulse << "\n";
            std::cout << "Normal impulse vector: " << glm::to_string(normalImpulse) << "\n";
        }

        vel.linear += invMass.value * normalImpulse;
        vel.angular += inertia.inverseInertiaTensorWorld * glm::cross(r, normalImpulse);

        if (loggingEnabled) {
            std::cout << "Linear velocity after normal impulse: " << glm::to_string(vel.linear)
                      << "\n";
            std::cout << "Angular velocity after normal impulse: " << glm::to_string(vel.angular)
                      << "\n";
        }

        // Recalculate velocity after normal impulse for friction
        velocityAtContact = vel.linear + glm::cross(vel.angular, r);
        normalVelocity = glm::dot(velocityAtContact, normal);

        glm::vec3 tangentialVelocity = velocityAtContact - normalVelocity * normal;
        float tangentialSpeed = glm::length(tangentialVelocity);

        if (loggingEnabled) {
            std::cout << "Tangential velocity: " << glm::to_string(tangentialVelocity) << "\n";
            std::cout << "Tangential speed: " << tangentialSpeed << "\n";
        }

        if (tangentialSpeed > 1e-6f) {
            glm::vec3 tangent = tangentialVelocity / tangentialSpeed;
            glm::vec3 rCrossT = glm::cross(r, tangent);
            glm::vec3 Iinv_rCrossT = inertia.inverseInertiaTensorWorld * rCrossT;
            float frictionDenominator = invMass.value + glm::dot(rCrossT, Iinv_rCrossT);

            if (loggingEnabled) {
                std::cout << "Friction denominator: " << frictionDenominator << "\n";
            }

            if (frictionDenominator > 1e-6f) {
                float jt = -tangentialSpeed / frictionDenominator;

                // Coulomb friction: tangential impulse is capped by the normal impulse
                // actually generated this contact, scaled by the friction coefficient.
                float maxFriction = frictionCoeff * accumulatedImpulse;
                jt = glm::clamp(jt, -maxFriction, maxFriction);

                glm::vec3 frictionImpulse = jt * tangent;

                if (loggingEnabled) {
                    std::cout << "Friction impulse scalar jt: " << jt << "\n";
                    std::cout << "Max friction: " << maxFriction << "\n";
                    std::cout << "Friction impulse vector: " << glm::to_string(frictionImpulse)
                              << "\n";
                }

                vel.linear += invMass.value * frictionImpulse;
                vel.angular += inertia.inverseInertiaTensorWorld * glm::cross(r, frictionImpulse);

                if (loggingEnabled) {
                    std::cout << "Linear velocity after friction: " << glm::to_string(vel.linear)
                              << "\n";
                    std::cout << "Angular velocity after friction: " << glm::to_string(vel.angular)
                              << "\n";
                }
            }
        }
    }

    bool needsPositionCorrection = penetrationDepth > slop;
    if (needsPositionCorrection && invMass.value > 0.0f) {
        float correctionMagnitude = percent * (penetrationDepth - slop);
        glm::vec3 positionCorrection = correctionMagnitude * normal;

        if (glm::length(positionCorrection) > maxLinearCorrection) {
            positionCorrection = glm::normalize(positionCorrection) * maxLinearCorrection;
        }

        if (loggingEnabled) {
            std::cout << "Position correction: " << glm::to_string(positionCorrection) << "\n";
        }

        pos.current += positionCorrection;

        glm::vec3 r = position - pos.current;
        glm::vec3 angularCorrection =
            inertia.inverseInertiaTensorWorld * glm::cross(r, positionCorrection * 0.1f);
        float angle = glm::length(angularCorrection);
        if (angle > 1e-6f) {
            glm::vec3 axis = angularCorrection / angle;
            glm::mat3 rot = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, axis));
            orient.rotationMatrix = rot * orient.rotationMatrix;
        }
    }

    if (loggingEnabled) {
        std::cout << "\n=== resolveHeightMapCollision END ===\n";
        std::cout << "Final position: " << glm::to_string(pos.current) << "\n";
        std::cout << "Final linear velocity: " << glm::to_string(vel.linear) << "\n";
        std::cout << "Final angular velocity: " << glm::to_string(vel.angular) << "\n";
        std::cout << "Final accumulated impulse: " << accumulatedImpulse << "\n\n";
    }
}
