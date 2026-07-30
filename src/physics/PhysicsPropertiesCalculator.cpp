#include "PhysicsPropertiesCalculator.hpp"

#include "collision/shapes/ConvexHull.hpp"
#include "collision/shapes/OBB.hpp"
#include "collision/shapes/Sphere.hpp"
#include "components/collision/ShapeData.hpp"
#include "components/physics/PhysicsComponents.hpp"
#include "core/Core.hpp"

template <typename BoundingCollisionType>
PhysicsProperties PhysicsPropertiesCalculator<BoundingCollisionType>::calculateBaseProperties(
    entt::entity& e) {
    throw std::runtime_error("Calculation not implemented for this bounding volume type");
}

PhysicsProperties PhysicsPropertiesCalculator<OBB>::calculateBaseProperties(entt::entity& e) {
    PhysicsProperties properties;
    properties.inertiaTensor = calculateInertiaTensorLocal(e);
    properties.principleAxes = calculatePrincipleAxes(properties.inertiaTensor);
    properties.centroid = calculateCentroid(e);
    return properties;
}

glm::mat3 PhysicsPropertiesCalculator<OBB>::calculateInertiaTensorLocal(entt::entity& e) {
    OBBData& obb = Registry.get<OBBData>(e);
    float& inverseMass = Registry.get<InverseMass>(e).value;
    float height = obb.max.x - obb.min.x;
    float width = obb.max.y - obb.min.y;
    float depth = obb.max.z - obb.min.z;

    float I_h = (1.0f / 12.0f) * (1 / inverseMass) * (width * width + depth * depth);
    float I_w = (1.0f / 12.0f) * (1 / inverseMass) * (depth * depth + height * height);
    float I_d = (1.0f / 12.0f) * (1 / inverseMass) * (width * width + height * height);

    glm::mat itl = glm::mat3(0.0f);
    itl[0][0] = I_h;
    itl[1][1] = I_w;
    itl[2][2] = I_d;

    return itl;
}

glm::mat3 PhysicsPropertiesCalculator<OBB>::calculatePrincipleAxes(glm::mat3 inertiaTensor) {
    glm::mat3 principleAxes = glm::mat3(1.0f);

    Eigen::Matrix3f inertiaMatrix = Eigen::Matrix3f::Zero();
    inertiaMatrix(0, 0) = inertiaTensor[0][0];
    inertiaMatrix(1, 1) = inertiaTensor[1][1];
    inertiaMatrix(2, 2) = inertiaTensor[2][2];

    // Perform eigendecomposition
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigensolver(inertiaMatrix);

    // Get the eigenvectors (principal axes)
    Eigen::Matrix3f eigenPrincipleAxes = eigensolver.eigenvectors();

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            // Note: Eigen is column-major by default, same as GLM
            principleAxes[j][i] = static_cast<float>(eigenPrincipleAxes(i, j));
        }
    }
    return principleAxes;
}

glm::vec3 PhysicsPropertiesCalculator<OBB>::calculateCentroid(entt::entity& e) {
    OBBData& obb = Registry.get<OBBData>(e);
    glm::vec3 centroid = glm::vec3(0.0f);
    for (int i = 0; i < 3; i++) {
        centroid[i] = (obb.max[i] + obb.min[i]) * 0.5f;
    }
    std::cout << "calculated centroid: " << glm::to_string(centroid) << std::endl;
    return centroid;
}

PhysicsProperties PhysicsPropertiesCalculator<ConvexHull>::calculateBaseProperties(
    entt::entity& e) {
    PhysicsProperties properties;
    properties.inertiaTensor = calculateInertiaTensorLocal(e);
    properties.principleAxes = calculatePrincipleAxes(properties.inertiaTensor);
    properties.centroid = calculateCentroid(e);
    return properties;
}

glm::mat3 PhysicsPropertiesCalculator<ConvexHull>::calculateInertiaTensorLocal(entt::entity& e) {
    ConvexHullData& convexHullData = Registry.get<ConvexHullData>(e);
    float& inverseMass = Registry.get<InverseMass>(e).value;
    float height = convexHullData.max.x - convexHullData.min.x;
    float width = convexHullData.max.y - convexHullData.min.y;
    float depth = convexHullData.max.z - convexHullData.min.z;

    float I_h = (1.0f / 12.0f) * (1 / inverseMass) * (width * width + depth * depth);
    float I_w = (1.0f / 12.0f) * (1 / inverseMass) * (depth * depth + height * height);
    float I_d = (1.0f / 12.0f) * (1 / inverseMass) * (width * width + height * height);

    glm::mat itl = glm::mat3(0.0f);
    itl[0][0] = I_h;
    itl[1][1] = I_w;
    itl[2][2] = I_d;

    return itl;
}

glm::mat3 PhysicsPropertiesCalculator<ConvexHull>::calculatePrincipleAxes(glm::mat3 inertiaTensor) {
    glm::mat3 principleAxes = glm::mat3(1.0f);

    Eigen::Matrix3f inertiaMatrix = Eigen::Matrix3f::Zero();
    inertiaMatrix(0, 0) = inertiaTensor[0][0];
    inertiaMatrix(1, 1) = inertiaTensor[1][1];
    inertiaMatrix(2, 2) = inertiaTensor[2][2];

    // Perform eigendecomposition
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigensolver(inertiaMatrix);

    // Get the eigenvectors (principal axes)
    Eigen::Matrix3f eigenPrincipleAxes = eigensolver.eigenvectors();

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            // Note: Eigen is column-major by default, same as GLM
            principleAxes[j][i] = static_cast<float>(eigenPrincipleAxes(i, j));
        }
    }
    return principleAxes;
}

glm::vec3 PhysicsPropertiesCalculator<ConvexHull>::calculateCentroid(entt::entity& e) {
    ConvexHullData& convexHullData = Registry.get<ConvexHullData>(e);
    return convexHullData.currentCenter;
}

PhysicsProperties PhysicsPropertiesCalculator<Sphere>::calculateBaseProperties(entt::entity& e) {
    PhysicsProperties properties;
    properties.inertiaTensor = calculateInertiaTensorLocal(e);
    properties.principleAxes = calculatePrincipleAxes(properties.inertiaTensor);
    properties.centroid = calculateCentroid(e);
    return properties;
}

glm::mat3 PhysicsPropertiesCalculator<Sphere>::calculateInertiaTensorLocal(entt::entity& e) {
    SphereData& sphere = Registry.get<SphereData>(e);
    float& inverseMass = Registry.get<InverseMass>(e).value;
    float radius = sphere.radius;

    float mass = 1.0f / inverseMass;  // assuming inverseMass is set
    float I = (2.0f / 5.0f) * mass * radius * radius;

    glm::mat3 itl(0.0f);
    itl[0][0] = I;
    itl[1][1] = I;
    itl[2][2] = I;

    return itl;
}

glm::mat3 PhysicsPropertiesCalculator<Sphere>::calculatePrincipleAxes(glm::mat3 inertiaTensor) {
    return glm::mat3(1.0f);
}

glm::vec3 PhysicsPropertiesCalculator<Sphere>::calculateCentroid(entt::entity& e) {
    SphereData& sphere = Registry.get<SphereData>(e);
    return sphere.currentCenter;
}

// Explicit instantiation of the template for types you'll use
template class PhysicsPropertiesCalculator<OBB>;
template class PhysicsPropertiesCalculator<ConvexHull>;
template class PhysicsPropertiesCalculator<Sphere>;
