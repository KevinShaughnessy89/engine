#pragma once
#include "core/PrecompiledHeader.hpp"
#include <glm/glm.hpp>
#include <vector>
#include "collision/CollisionAliases.hpp"

enum class FeatureID {
    // Vertices (8)
    AABB_VERTEX_000, AABB_VERTEX_001, AABB_VERTEX_010, AABB_VERTEX_011,
    AABB_VERTEX_100, AABB_VERTEX_101, AABB_VERTEX_110, AABB_VERTEX_111,
    
    // Edges (12)
    AABB_EDGE_X_00, AABB_EDGE_X_01, AABB_EDGE_X_10, AABB_EDGE_X_11,
    AABB_EDGE_Y_00, AABB_EDGE_Y_01, AABB_EDGE_Y_10, AABB_EDGE_Y_11,
    AABB_EDGE_Z_00, AABB_EDGE_Z_01, AABB_EDGE_Z_10, AABB_EDGE_Z_11,
    
    // Faces (6)
    AABB_FACE_X_NEG, AABB_FACE_X_POS,
    AABB_FACE_Y_NEG, AABB_FACE_Y_POS,
    AABB_FACE_Z_NEG, AABB_FACE_Z_POS,
    DEFAULT
};

class ContactDetails {
    public:
        virtual ~ContactDetails() = default;
        virtual glm::vec3 calculateIntersectionNormal(const glm::vec3& intersectionPoint) const = 0;
        virtual float calculatePenetrationDepth(const CollisionResultPtr collisionData) const = 0;
        virtual glm::vec3 getOppositeEdgeCenter(const std::vector<glm::vec3>& points, const glm::vec3& normal) const = 0;
        virtual std::vector<int> getFurthestVertices(const glm::vec3& position) const = 0;
        virtual glm::vec3 getVertexAtIndex(int index) const = 0;
        virtual FeatureID getFeatureID(const glm::vec3& point) const = 0;
};
