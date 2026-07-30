// #include "ConvexHullContactDetails.hpp"

// #include "collision/ContactResolver.hpp"
// #include "collision/shapes/AABB.hpp"
// #include "collision/shapes/ConvexHull.hpp"
// #include "collision/shapes/OBB.hpp"
// #include "collision/shapes/Triangle.hpp"
// #include "collision/shapes/geometry/AABBGeometry.hpp"
// #include "collision/shapes/geometry/ConvexGeometry.hpp"
// #include "collision/shapes/geometry/GeometryObject.hpp"
// #include "collision/shapes/geometry/OBBGeometry.hpp"
// #include "core/PrecompiledHeader.hpp"

// bool ConvexHullContactDetails::faceAABBIntersection(const TriangleData& face,
//                                                     const AABBData& aabb) {
//     // Get face vertices
//     glm::vec3 v0 = face.vertices[0];
//     glm::vec3 v1 = face.vertices[1];
//     glm::vec3 v2 = face.vertices[2];

//     // Face plane equation: dot(n, X) + d = 0
//     glm::vec3 n = face.normal;
//     float d = -glm::dot(n, v0);

//     // Project OBB onto face normal - radius of obb projection on plane
//     float r = aabb.halfExtents.x * std::abs(n.x) + aabb.halfExtents.y * std::abs(n.y) +
//               aabb.halfExtents.z * std::abs(n.z);

//     // Compute distance of OBB center to face plane
//     float s = glm::dot(n, aabb.currentCenter) + d;

//     // No intersection if OBB is too far from face plane
//     if (std::abs(s) > r) return false;

//     // Compute intersection point
//     glm::vec3 intersectionPoint = aabb.currentCenter - s * n;

//     // Check if intersection point is inside the face
//     glm::vec3 edge0 = v1 - v0;
//     glm::vec3 edge1 = v2 - v1;
//     glm::vec3 edge2 = v0 - v2;

//     glm::vec3 c0 = intersectionPoint - v0;
//     glm::vec3 c1 = intersectionPoint - v1;
//     glm::vec3 c2 = intersectionPoint - v2;
//     // Perform inside-outside test.
//     //  checks if the intersection point is on the same side of all three edges. If it is, the
//     point
//     //  is inside the triangle.
//     if (glm::dot(n, glm::cross(edge0, c0)) > 0 && glm::dot(n, glm::cross(edge1, c1)) > 0 &&
//         glm::dot(n, glm::cross(edge2, c2)) > 0) {
//         return true;
//     }

//     return false;
// }

// bool ConvexHullContactDetails::edgeEdgeIntersection(const glm::vec3& p1, const glm::vec3& p2,
//                                                     const glm::vec3& p3, const glm::vec3& p4,
//                                                     glm::vec3& intersectionPoint) {
//     // create parametric equations of edges. P(t) = v1 + t(v2-v1) (offset and direction vectors)
//     // were looking for t and u values that minimize the distance between the two lines -> that's
//     // the point of contact
//     glm::vec3 d1 = p2 - p1;  // Direction vector of edge1
//     glm::vec3 d2 = p4 - p3;  // Direction vector of edge2

//     // Check if edges are parallel
//     glm::vec3 n = glm::cross(d1, d2);
//     if (glm::length2(n) < 1e-6f) return false;  // Edges are parallel

//     // Set the constraint that the vector between the closet points should be perpendicular to
//     both
//     // lines (minimize distance between lines): (P(t)-(Q(t))).d1 = 0 and (P(t)-(Q(t))).d2 = 0
//     (P(t)
//     // - Q(t) -> generalized vector from point on one line to point on the other)
//     // substitute and solve using Cramer's Rule (and vector triple product identity)
//     // in simplified form:
//     glm::vec3 p13 = p1 - p3;
//     float t = glm::dot(glm::cross(p13, d2), n) / glm::length2(n);
//     float u = glm::dot(glm::cross(p13, d1), n) / glm::length2(n);

//     // Check if intersection point is within both edge segments
//     if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
//         intersectionPoint = p1 + t * d1;
//         return true;
//     }

//     return false;
// }

// float ConvexHullContactDetails::calculatePenetrationDepthAABB(const ConvexHullData& hull,
//                                                               const AABBData& aabb) {
//     static const std::array<glm::vec3, 3> principalAxes = {
//         glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

//     float minPenetration = std::numeric_limits<float>::max();

//     // Check hull face normals
//     for (const auto& face : hull->currentFaces) {
//         float penetration = calculatePenetrationAlongAxisAABB(hull, aabb, face.normal);
//         if (penetration < 0) return -1;  // Separating axis found
//         minPenetration = std::min(minPenetration, penetration);
//     }

//     // Check AABB principal axes
//     for (const auto& axis : principalAxes) {
//         float penetration = calculatePenetrationAlongAxisAABB(hull, aabb, axis);
//         if (penetration < 0) return -1;  // Separating axis found
//         minPenetration = std::min(minPenetration, penetration);
//     }

//     return minPenetration;
// }

// float ConvexHullContactDetails::calculatePenetrationAlongAxisAABB(const ConvexHullData& hull,
//                                                                   const AABBData& aabb,
//                                                                   const glm::vec3& axis) {
//     float minA, maxA, minB, maxB;
//     ConvexGeometry::projectHullOntoAxis(hull, axis, minA, maxA);
//     AABBGeometry::projectAABBOntoAxis(aabb, axis, minB, maxB);

//     return std::min(maxA - minB, maxB - minA);
// }

// float ConvexHullContactDetails::calculatePenetrationDepthOBB(const ConvexHullData& hull,
//                                                              const OBBData& obb) {
//     float minPenetration = std::numeric_limits<float>::max();

//     // Check hull face normals
//     for (const auto& face : hull->currentFaces) {
//         float penetration = calculatePenetrationAlongAxisOBB(hull, obb, face.normal);
//         if (penetration < 0) return -1;  // Separating axis found
//         minPenetration = std::min(minPenetration, penetration);
//     }

//     // Check OBB axes
//     for (const auto& axis : obb.currentAxes) {
//         float penetration = calculatePenetrationAlongAxisOBB(hull, obb, axis);
//         if (penetration < 0) return -1;  // Separating axis found
//         minPenetration = std::min(minPenetration, penetration);
//     }

//     // Check cross products of hull edges with OBB edges
//     const auto& hullEdges = ConvexGeometry::getEdges(hull);
//     const auto& obbEdges = OBBGeometry::getEdges(obb);
//     for (const auto& hullEdge : hullEdges) {
//         for (const auto& obbEdge : obbEdges) {
//             glm::vec3 axis = glm::cross(hullEdge, obbEdge);
//             if (glm::length2(axis) < 1e-6f) continue;  // Skip parallel edges
//             axis = glm::normalize(axis);
//             float penetration = calculatePenetrationAlongAxisOBB(hull, obb, axis);
//             if (penetration < 0) return -1;  // Separating axis found
//             minPenetration = std::min(minPenetration, penetration);
//         }
//     }

//     return minPenetration;
// }

// float ConvexHullContactDetails::calculatePenetrationAlongAxisOBB(const ConvexHullData& hull,
//                                                                  const OBBData& obb,
//                                                                  const glm::vec3& axis) {
//     float minA, maxA, minB, maxB;
//     ConvexGeometry::projectHullOntoAxis(hull, axis, minA, maxA);
//     OBBGeometry::projectOBBOntoAxis(obb, axis, minB, maxB);

//     float penetration = std::min(maxA - minB, maxB - minA);
//     return penetration;
// }

// float ConvexHullContactDetails::calculateEdgeEdgePenetrationDepth(
//     const glm::vec3& intersectionPoint, const glm::vec3& edge1Start, const glm::vec3& edge1End,
//     const glm::vec3& edge2Start, const glm::vec3& edge2End) {
//     float dist1 = glm::distance(intersectionPoint, GeometryObject::closestPointOnLine(
//                                                        intersectionPoint, edge1Start, edge1End));
//     float dist2 = glm::distance(intersectionPoint, GeometryObject::closestPointOnLine(
//                                                        intersectionPoint, edge2Start, edge2End));
//     return std::min(dist1, dist2);
// }

// float ConvexHullContactDetails::getPointPenetrationDepth(const ConvexHullData& hull,
//                                                          const OBBData& obb, const glm::vec3&
//                                                          point, const glm::vec3& collisionNormal)
//                                                          {
//     float min, max;
//     float hullProjection = glm::dot(point, collisionNormal);
//     OBBGeometry::projectOBBOntoAxis(obb, collisionNormal, min, max);

//     float penetrationMax = max - hullProjection;
//     float penetrationMin = hullProjection - min;

//     return std::max({0.0f, penetrationMax, penetrationMin});
// }
