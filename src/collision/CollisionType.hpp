#pragma once

enum class CollisionType {
    ConvexHull,
    AABB,
    OBB,
    Sphere,
    TerrainTriangle,
    Plane,
    Triangle,
    Capsule
};