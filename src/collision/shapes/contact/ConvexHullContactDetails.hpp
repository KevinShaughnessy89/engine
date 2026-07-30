// #pragma once
// #include "collision/CollisionAliases.hpp"
// #include "components/collision/ShapeData.hpp"
// #include "core/PrecompiledHeader.hpp"

// class ConvexHull;
// class OBB;
// class AABB;
// class ConvexHull;
// struct CollisionResult;
// class Triangle;

// class ConvexHullContactDetails {
//    public:
//     ConvexHullContactDetails() {}
//     static glm::vec3 calculateContactNormal(ConvexHullData& hull,
//                                             const glm::vec3& intersectionPoint);
//     static float calculatePenetrationDepth(ConvexHullData& hull,
//                                            const CollisionResultPtr collisionData);
//     static glm::vec3 getOppositeEdgeCenter(ConvexHullData& hull,
//                                            const std::vector<glm::vec3>& points,
//                                            const glm::vec3& normal);
//     static std::vector<int> getFurthestVertices(ConvexHullData& hull, const glm::vec3& position);
//     static glm::vec3 getVertexAtIndex(ConvexHullData& hull, int index);
//     // static FeatureID getFeatureID(ConvexHullData& hull, const glm::vec3& point);
//     static bool faceAABBIntersection(const TriangleData& face, const AABBData& aabb);
//     static bool edgeEdgeIntersection(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3&
//     p3,
//                                      const glm::vec3& p4, glm::vec3& intersectionPoint);
//     static float getPointPenetrationDepth(const ConvexHullData& hull, const OBBData& obb,
//                                           const glm::vec3& point, const glm::vec3&
//                                           collisionNormal);
//     static float calculatePenetrationDepthOBB(const ConvexHullData& hull, const OBBData& obb);
//     static float calculatePenetrationAlongAxisOBB(const ConvexHullData& hull, const OBBData& obb,
//                                                   const glm::vec3& axis);
//     static float calculatePenetrationDepthAABB(const ConvexHullData& hull, const AABBData& aabb);
//     static float calculatePenetrationAlongAxisAABB(const ConvexHullData& hull, const AABBData&
//     aabb,
//                                                    const glm::vec3& axis);
//     static float calculateEdgeEdgePenetrationDepth(const glm::vec3& intersectionPoint,
//                                                    const glm::vec3& edge1Start,
//                                                    const glm::vec3& edge1End,
//                                                    const glm::vec3& edge2Start,
//                                                    const glm::vec3& edge2End);
// };
