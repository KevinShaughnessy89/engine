#include "PhysicsManager.hpp"

#include "GravityForceGenerator.hpp"
#include "ProjectileForceGenerator.hpp"
#include "collision/CollisionType.hpp"
#include "collision/shapes/OBB.hpp"
#include "collision/shapes/Sphere.hpp"
#include "components/collision/ShapeData.hpp"
#include "components/physics/JustCreated.hpp"
#include "components/physics/PhysicsComponents.hpp"
#include "core/Core.hpp"
#include "entt-main/src/entt/entity/fwd.hpp"
#include "entt-main/src/entt/entt.hpp"

void PhysicsManager::init() {
}

void PhysicsManager::applyGravity(entt::entity e, float duration) {
    gravityForceGenerator.updateForce(Registry, e, duration);
}

void PhysicsManager::applyProjectileForce(glm::vec3 position, glm::vec3 direction,
                                          float forceMagnitude, entt::entity e) {
    ProjectileForceGenerator pf(position, direction, forceMagnitude);

    pf.updateForce(Registry, e, 0.0f);
}

void PhysicsManager::applyForceOnObject(entt::entity e, const glm::vec3& force) {
    auto& f = Registry.get<Force>(e);
    f.accumulated += force;
}

void PhysicsManager::runPhysics(float duration) {
    auto view = Registry.view<Position, Velocity, Acceleration, Force, InverseMass, Torque, Inertia,
                              Orientation, Sleep, Damping>();

    for (auto e : view) {
        if (Registry.all_of<JustCreated>(e)) continue;
        applyGravity(e, duration);
    }

    objectForceRegistry.updateForces(Registry, duration);

    for (auto e : view) {
        if (Registry.all_of<JustCreated>(e)) continue;
        update(e, duration);
    }
}

// ResolvedConstraint PhysicsManager::resolveKinematicMove(
//     glm::vec3& newPosition,
//     glm::vec3& newVelocity,
//     CollisionShape boundingVolume)
// {
//     std::vector<ContactManifoldPtr> results =
//         Collision.checkIntersectionsAtPoint(newPosition, boundingVolume);

//     return kinematicResolver.resolveCollisionConstraint(
//         newPosition, newVelocity, results);
// }

static constexpr float VELOCITY_EPSILON = 2.5f;
static constexpr float ANGULAR_VELOCITY_EPSILON = 1.0f;
static constexpr float VELOCITY_CHANGE_EPSILON_SQUARED = 0.5f;
static constexpr float SLEEP_THRESHOLD = 0.004f * 5.0f;

void PhysicsManager::updateIitw(Orientation& rot, Inertia& i) {
    i.inverseInertiaTensorWorld =
        rot.rotationMatrix * i.inverseInertiaTensorLocal * glm::transpose(rot.rotationMatrix);
}

glm::vec3 PhysicsManager::localToWorldPoint(const Position& pos, const Orientation& rot,
                                            glm::vec3 localPoint) {
    return pos.current + rot.rotationMatrix * localPoint;
}

glm::vec3 PhysicsManager::localToWorldVector(const Orientation& rot, glm::vec3 localVector) {
    return rot.rotationMatrix * localVector;
}

glm::vec3 PhysicsManager::worldToLocalPoint(const Position& pos, const Orientation& rot,
                                            glm::vec3 worldPoint) {
    return glm::transpose(rot.rotationMatrix) * (worldPoint - pos.current);
}

glm::vec3 PhysicsManager::worldToLocalVector(const Orientation& rot, glm::vec3 worldVector) {
    return glm::transpose(rot.rotationMatrix) * worldVector;
}

glm::vec3 PhysicsManager::getVelocityAtPoint(const Velocity& vel, const glm::vec3& angularVelocity,
                                             const Position& pos, const glm::vec3& point) {
    return vel.linear + glm::cross(vel.angular, point - pos.current);
}

void PhysicsManager::addForce(entt::entity e, const glm::vec3& force) {
    auto& f = Registry.get<Force>(e);
    auto& s = Registry.get<Sleep>(e);
    f.accumulated += force;
    s.forceApplied = true;
}

void PhysicsManager::addForceAtPoint(entt::entity e, glm::vec3 worldPoint, glm::vec3 worldForce) {
    auto& pos = Registry.get<Position>(e);
    auto& force = Registry.get<Force>(e);
    auto& torque = Registry.get<Torque>(e);
    auto& sleep = Registry.get<Sleep>(e);

    auto& shape = Registry.get<ShapeData>(e);

    glm::vec3 contactNormal = glm::vec3(0.f);
    if (shape.type == CollisionType::OBB) {
        OBBData& data = std::get<OBBData>(shape.data);
        contactNormal = OBB::calculateContactNormal(data, worldPoint);

    } else if (shape.type == CollisionType::Sphere) {
        SphereData& data = std::get<SphereData>(shape.data);
        contactNormal = Sphere::calculateContactNormal(data, worldPoint);
    } else {
        // Throw error
    }

    float normalForceMagnitude =
        glm::dot(glm::normalize(worldForce), contactNormal) * glm::length(worldForce);

    glm::vec3 normalForce = normalForceMagnitude * contactNormal;
    glm::vec3 tangentialForce = (worldForce - normalForce) * 0.10f;

    force.accumulated += normalForce + tangentialForce;

    glm::vec3 momentArm = worldPoint - pos.current;
    glm::vec3 worldTorque = glm::cross(momentArm, worldForce);

    torque.worldTorqueAccumulated += worldTorque;

    sleep.forceApplied = true;
}

bool PhysicsManager::shouldSleep(entt::entity e, float duration) {
    auto& vel = Registry.get<Velocity>(e);
    auto& force = Registry.get<Force>(e);
    auto& invMass = Registry.get<InverseMass>(e);

    bool velocityNearZero = glm::length(vel.linear) < VELOCITY_EPSILON &&
                            glm::length(vel.angular) < ANGULAR_VELOCITY_EPSILON;

    bool isRestingContact =
        glm::length2(vel.linear - (force.accumulated * invMass.value * duration)) <
        VELOCITY_CHANGE_EPSILON_SQUARED;

    return velocityNearZero && isRestingContact;
}

void PhysicsManager::update(entt::entity e, float duration) {
    bool shouldSleepNow = shouldSleep(e, duration);

    auto& sleep = Registry.get<Sleep>(e);
    auto& vel = Registry.get<Velocity>(e);
    auto& force = Registry.get<Force>(e);
    auto& torque = Registry.get<Torque>(e);

    if (shouldSleepNow) {
        sleep.sleepTimer += duration;
    } else
        sleep.sleepTimer = 0.0f;

    if (sleep.sleepTimer > SLEEP_THRESHOLD) sleep.isAwake = false;

    if (sleep.isAwake) {
        integrate(e, duration);
        return;
    }

    if (sleep.forceApplied) {
        sleep.isAwake = true;
        sleep.sleepTimer = 0.0f;
        integrate(e, duration);
        return;
    }

    vel.linear = glm::vec3(0.0f);
    vel.angular = glm::vec3(0.0f);

    force.accumulated = glm::vec3(0.0f);
    torque.worldTorqueAccumulated = glm::vec3(0.0f);
}

void PhysicsManager::integrate(entt::entity e, float duration) {
    auto& position = Registry.get<Position>(e);
    auto& acceleration = Registry.get<Acceleration>(e);
    auto& velocity = Registry.get<Velocity>(e);
    auto& force = Registry.get<Force>(e);
    auto& torque = Registry.get<Torque>(e);
    auto& inertia = Registry.get<Inertia>(e);
    auto& inverseMass = Registry.get<InverseMass>(e);
    auto& damping = Registry.get<Damping>(e);
    auto& orientation = Registry.get<Orientation>(e);

    // Calculate total acceleration for this frame
    glm::vec3 totalAcceleration = acceleration.linear + force.accumulated * inverseMass.value;
    glm::vec3 totalAngularAcceleration =
        acceleration.angular + inertia.inverseInertiaTensorLocal *
                                   worldToLocalVector(orientation, force.worldTorqueAccumulated);

    // Update velocities
    velocity.linear += totalAcceleration * duration;
    velocity.angular += totalAngularAcceleration * duration;

    // Apply damping
    velocity.linear *= pow(damping.linear, duration);
    velocity.angular *= pow(damping.angular, duration);

    // Update positions
    glm::vec3 deltaPosition = velocity.linear * duration;
    glm::vec3 deltaAngularPosition = velocity.angular * duration;

    translate(position, orientation, deltaPosition);
    rotate(orientation, deltaAngularPosition);
    updateIitw(orientation, inertia);

    // Clear forces
    force.accumulated = glm::vec3(0.0f);
    torque.worldTorqueAccumulated = glm::vec3(0.0f);
    torque.localTorqueAccumulated = glm::vec3(0.0f);
}

void PhysicsManager::translate(Position& position, Orientation& orientation,
                               const glm::vec3& translation) {
    position.previous = position.current;
    position.current += translation;
    orientation.translationMatrix = glm::translate(glm::mat4(1.0f), position.current);
}

void PhysicsManager::rotate(Orientation& orientation, const glm::vec3& rotationAngle) {
    if (glm::length(rotationAngle) < Config::EPSILON) {
        return;
    }

    float angle = glm::length(rotationAngle);
    glm::vec3 axis = glm::normalize(rotationAngle);

    glm::mat3 deltaRotation = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, axis));

    orientation.rotationMatrix = orientation.rotationMatrix * deltaRotation;

    // Re-orthonormalize
    orientation.rotationMatrix[0] = glm::normalize(glm::vec3(orientation.rotationMatrix[0]));
    orientation.rotationMatrix[2] =
        glm::normalize(glm::cross(orientation.rotationMatrix[0], orientation.rotationMatrix[1]));
    orientation.rotationMatrix[1] =
        glm::cross(orientation.rotationMatrix[2], orientation.rotationMatrix[0]);
}
