#pragma once

struct ContactPoint {
    ContactPoint(glm::vec3 position, glm::vec3 normal, float penetrationDepth)
        : position(position), normal(normal), penetrationDepth(penetrationDepth) {}
    glm::vec3 position;
    glm::vec3 normal;
    float penetrationDepth;
    float frictionCoefficient;
    float accumulatedImpulse;
    // FeatureID idBodyA;
    // FeatureID idBodyB;
    int numFrames = 0;

    ContactPoint() {
        position = glm::vec3(0.0f);
        normal = glm::vec3(0.0f);
        penetrationDepth = 0.0f;
        frictionCoefficient = 0.0f;
        float accumulatedImpulse = 0.0f;
        // FeatureID idBodyA = FeatureID::DEFAULT;
        // FeatureID idBodyB = FeatureID::DEFAULT;
        numFrames = 0;
    }

    bool operator==(const ContactPoint& other) const {
        const float positionTolerance = 0.1f;  // Adjust as needed
        const float normalTolerance = 0.1f;    // Adjust as needed
        const float depthTolerance = 0.1f;     // Adjust as needed

        if (this->hash() != other.hash()) {
            return false;
        }

        bool positionClose = glm::distance(position, other.position) < positionTolerance;
        bool normalClose = glm::dot(normal, other.normal) > (1.0f - normalTolerance);
        bool depthClose = std::abs(penetrationDepth - other.penetrationDepth) < depthTolerance;

        return positionClose && normalClose && depthClose;
    }

    size_t hash() const {
        const float positionTolerance = 0.1f;  // Doubled the observed range
        const float normalTolerance = 0.1f;    // Assuming similar tolerance for normals
        const float depthTolerance = 0.1f;     // Assuming similar tolerance for depth

        size_t h = 0;
        auto roundToTolerance = [](float value, float tolerance) {
            return std::round(value / tolerance) * tolerance;
        };
        for (int i = 0; i < 3; ++i) {
            float roundedPos = roundToTolerance(position[i], positionTolerance);
            h ^= std::hash<float>{}(roundedPos) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        for (int i = 0; i < 3; ++i) {
            float roundedNormal = roundToTolerance(normal[i], normalTolerance);
            h ^= std::hash<float>{}(roundedNormal) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        float roundedDepth = roundToTolerance(penetrationDepth, depthTolerance);
        h ^= std::hash<float>{}(roundedDepth) + 0x9e3779b9 + (h << 6) + (h >> 2);

        return h;
    }
};