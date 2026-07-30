// #pragma once
// #include "collision/QuickHull/QuickHull.hpp"
// #include "collision/shapes/geometry/GeometryObject.hpp"
// #include "core/PrecompiledHeader.hpp"

// class Model;
// class PhysicsObject;
// class Triangle;
// class ConvexHull;

// struct Vector3Hash {
//     std::size_t operator()(const glm::vec3& v) const {
//         std::size_t h1 = std::hash<float>()(v.x);
//         std::size_t h2 = std::hash<float>()(v.y);
//         std::size_t h3 = std::hash<float>()(v.z);
//         return h1 ^ (h2 << 1) ^ (h3 << 2);
//     }
// };

// namespace std {
// template <>
// struct hash<glm::vec3> {
//     std::size_t operator()(const glm::vec3& v) const {
//         // Use the hash function defined in MyKey
//         return Vector3Hash()(v);
//     }
// };

// inline bool operator==(const glm::vec3& v1, const glm::vec3& v2) {
//     return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
// }

// }  // namespace std

// class ConvexGeometry : public GeometryObject {
//    public:
//     ConvexGeometry();
//     static void buildHull(ConvexHullData& hull);
//     static bool buildMeshComponents(ConvexHullData& hull);
//     static void createAdjacencyList(ConvexHullData& hull);
//     static void weldVerticesAndUpdateIndices(ConvexHullData& hull, float threshold);
//     static void postProcessing(ConvexHullData& hull);
//     static float calculateSolidAngle(const ConvexHullData& hull, int vertexIndex,
//                                      const TriangleData& triangle);
//     static bool isCornerVertex(const ConvexHullData& hull, int vertexIndex);
//     static void projectHullOntoAxis(const ConvexHullData& hull, const glm::vec3& axis, float&
//     min,
//                                     float& max);
//     static glm::vec3 calculateEdgeEdgeNormal(const glm::vec3& edge1Start, const glm::vec3&
//     edge1End,
//                                              const glm::vec3& edge2Start,
//                                              const glm::vec3& edge2End);
//     static std::vector<glm::vec3> getEdges(const ConvexHullData& hull);
//     static glm::vec3 findNormalAtPoint(const ConvexHullData& hull, const glm::vec3& point);
// };
