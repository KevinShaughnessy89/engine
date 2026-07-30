// #include "ConvexHull.hpp"

// #include <deque>

// #include "collision/CollisionAliases.hpp"
// #include "collision/shapes/AABB.hpp"
// #include "collision/shapes/OBB.hpp"
// #include "collision/shapes/Plane.hpp"
// #include "collision/shapes/Ray.hpp"
// #include "collision/shapes/Triangle.hpp"
// #include "collision/shapes/contact/ConvexHullContactDetails.hpp"
// #include "collision/shapes/geometry/ConvexGeometry.hpp"
// #include "collision/shapes/geometry/TriangleGeometry.hpp"
// #include "core/PrecompiledHeader.hpp"

// ConvexHull::ConvexHull() {
// }

// void ConvexHull::defineBoundingVolume(const ConvexHullData& hull, GeometricData geoData) {
//     quickhull::QuickHull<float> qh;

//     auto& hull = qh.getConvexHull(geoData.pointCloud, true, false);

//     const auto& hullVertices = hull.getVertexBuffer();
//     const auto& hullIndices = hull.getIndexBuffer();

//     for (int i = 0; i < hullVertices.size(); i++) {
//         static_assert(std::is_same_v<decltype(hullVertices[0].x), float>, "x should be float");
//         glm::vec3 vertex = glm::vec3(hullVertices[i].x, hullVertices[i].y, hullVertices[i].z);
//         this->vertices.push_back(vertex);
//     }

//     for (const auto& index : hullIndices) {
//         this->indices.push_back(index);
//     }

//     glm::vec3 modelMax = geoData.max;
//     glm::vec3 modelMin = geoData.min;

//     // Calculate the center
//     this->initialCenter =
//         glm::vec3((modelMax.x + modelMin.x) / 2.0f, (modelMax.y + modelMin.y) / 2.0f,
//                   (modelMax.z + modelMin.z) / 2.0f);
//     this->currentCenter = this->initialCenter;
//     this->max = geoData.max;
//     this->min = geoData.min;

//     ConvexGeometry::buildHull(this);
// }

// void ConvexHull::update(Model* model) {
//     currentFaces.clear();
//     currentFaces.reserve(this->initialFaces.size());

//     for (int j = 0; j < this->initialFaces.size(); j++) {
//         TriangleData& newFace = new Triangle();
//         currentFaces.push_back(newFace);

//         for (int i = 0; i < 3; i++) {
//             currentFaces[j]->vertices[i] =
//                 glm::vec3(model->modelMatrix *
//                 glm::vec4(this->initialFaces[j]->vertices[i], 1.0f));
//             currentFaces[j]->indices[i] = this->initialFaces[j]->indices[i];
//         }

//         TriangleGeometry::calculateTriangleMinMax(currentFaces[j]);
//         TriangleGeometry::calculateNormal(currentFaces[j]);
//     }

//     currentCenter = glm::vec3(model->modelMatrix * glm::vec4(initialCenter, 1.0f));
// }

// void ConvexHull::render(const ShaderProgram* shader) {
//     GLuint VAOFaces = 0, VBOFaces = 0, VAONormals = 0, VBONormals = 0;

//     // m_expansionCounter++;
//     // if (m_expansionCounter >= EXPANSION_INTERVAL && !m_hullComplete) {
//     //     stepExpansion();
//     //     m_expansionCounter = 0;
//     // }
//     glGenVertexArrays(1, &VAOFaces);
//     glGenBuffers(1, &VBOFaces);

//     // glGenVertexArrays(1, &VAONormals);
//     // glGenBuffers(1, &VBONormals);

//     std::vector<glm::vec3> renderFaces;

//     for (const auto& face : this->initialFaces) {
//         renderFaces.push_back(face->vertices[0]);
//         renderFaces.push_back(face->vertices[1]);
//         renderFaces.push_back(face->vertices[2]);
//     }

//     glBindVertexArray(VAOFaces);
//     glBindBuffer(GL_ARRAY_BUFFER, VBOFaces);

//     glBufferData(GL_ARRAY_BUFFER, renderFaces.size() * sizeof(glm::vec3), renderFaces.data(),
//                  GL_STATIC_DRAW);
//     glEnableVertexAttribArray(0);
//     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

//     // std::vector<glm::vec3> currentNormals;
//     // //     for (const auto& face : m_openFaces) {
//     // //     glm::vec3 startLine = (face->vertices[0]] + face->vertices[1]] +
//     face->vertices[2]]) *
//     // 0.333f;
//     // //     currentNormals.push_back(startLine);
//     // //     currentNormals.push_back(face->normal * 5.0f);
//     // // }
//     // for (const auto& face : currentFaces) {
//     //     glm::vec3 startLine = (face->vertices[0] + face->vertices[1] + face->vertices[2]) *
//     //     0.333f; currentNormals.push_back(startLine); currentNormals.push_back(face->normal
//     //     * 5.0f);
//     // }

//     // glBindVertexArray(VAONormals);
//     // glBindBuffer(GL_ARRAY_BUFFER, VBONormals);

//     // glBufferData(GL_ARRAY_BUFFER, currentNormals.size()*sizeof(glm::vec3),
//     currentNormals.data(),
//     // GL_STATIC_DRAW); glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT,
//     // GL_FALSE, sizeof(glm::vec3), (void*)0);

//     glBindVertexArray(VAOFaces);
//     glBindBuffer(GL_ARRAY_BUFFER, VBOFaces);
//     glDrawArrays(GL_TRIANGLES, 0, renderFaces.size());

//     // glBindVertexArray(VAONormals);
//     // glBindBuffer(GL_ARRAY_BUFFER, VBONormals);
//     // glLineWidth(5.0f);  // Make lines thicker for visibility
//     // glDrawArrays(GL_LINES, 0, currentNormals.size());
//     // glLineWidth(1.0f);

//     glBindVertexArray(0);
//     glBindBuffer(GL_ARRAY_BUFFER, 0);

//     //     std::cout << "Rendering " << initialFaceRender.size() << " currentFaces:" <<
//     std::endl;
//     // for (size_t i = 0; i < initialFaceRender.size(); ++i) {
//     //     const std::unique_ptr<Face> face = initialFaceRender[i];
//     //     std::cout << "Face " << i << ": "
//     //               << "(" << face->vertices[0]].x << ", " << face->vertices[0]].y << ", " <<
//     //               face->vertices[0]].z << ") "
//     //               << "(" << face->vertices[1]].x << ", " << face->vertices[1]].y << ", " <<
//     //               face->vertices[1]].z << ") "
//     //               << "(" << face->vertices[2]].x << ", " << face->vertices[2]].y << ", " <<
//     //               face->vertices[2]].z << ")"
//     //               << std::endl;

//     //     std::cout << "   Normal: "
//     //               << "(" << face->normal.x << ", " << face->normal.y << ", " << face->normal.z
//     <<
//     //               ")"
//     //               << std::endl;
//     // }
//     // std::cout << std::endl;  // Add an extra line for readability between frames
// }

// CollisionResultPtr ConvexHull::collideWith(const CollisionShape* other) const {
//     // return other->collidWithConvexHull(*this);
//     CollisionResultPtr result = new CollisionResult();
//     // Implement collision logic here
//     return result;
// }

// CollisionResultPtr ConvexHull::collideWithPlane(const PlaneData& plane) const {
//     CollisionResultPtr result = new CollisionResult();
//     bool hasPositive = false;
//     bool hasNegative = false;
//     float closestDistance = std::numeric_limits<float>::max();
//     const glm::vec3* closestPoint = nullptr;

//     for (const auto& vertex : this->vertices) {
//         float distance = glm::dot(plane->normal, vertex) - plane->offset;
//         if (std::abs(distance) < Config::EPSILON) {
//             result->collided = true;
//             result->contactPoint.position = vertex;
//             result->contactPoint.normal = plane->normal;
//             return result;
//         }
//         if (distance > 0) {
//             hasPositive = true;
//         } else if (distance < 0) {
//             hasNegative = true;
//         }
//         // Keep track of the closest point to the plane
//         if (std::abs(distance) < std::abs(closestDistance)) {
//             closestDistance = distance;
//             closestPoint = &vertex;
//         }
//         if (hasPositive && hasNegative) {
//             // Hull intersects the plane
//             result->collided = true;
//             result->contactPoint.position = *closestPoint - plane->normal * closestDistance;
//             result->contactPoint.normal = plane->normal;
//             break;
//         }
//     }
//     return result;
// }

// CollisionResultPtr ConvexHull::collideWithAABB(const AABBData& aabb) const {
//     CollisionResultPtr result = new CollisionResult();

//     for (const auto& vertex : this->vertices) {
//         if (AABBGeometry::containsPoint(aabb, vertex)) {
//             result->collided = true;
//             result->contactPoint.position = vertex;
//             result->contactPoint.normal = AABBContactDetails::calculateContactNormal(aabb,
//             vertex); return result;
//         }
//     }
//     return result;
// }

// CollisionResultPtr ConvexHull::collideWithTriangle(const TriangleData& triangle) const {
//     CollisionResultPtr result = new CollisionResult();

//     // std::cout << "Triangle min: " << triangle->min.x << ", " << triangle->min.y << ", " <<
//     // triangle->min.z << std::endl; std::cout << "Triangle max: " << triangle->max.x << ", " <<
//     // triangle->max.y << ", " << triangle->max.z << std::endl; std::cout << "hull position: " <<
//     // glm::to_string(currentCenter) << std::endl; Function to test if there's a separating axis
//     auto test_axis = [&](const glm::vec3& axis) {
//         // std::cout << "Testing axis against hull: " << axis.x << ", " << axis.y << ", " <<
//         axis.z
//         // << std::endl;
//         float min1 = std::numeric_limits<float>::max(), max1 =
//         -std::numeric_limits<float>::max(); float min2 = std::numeric_limits<float>::max(), max2
//         = -std::numeric_limits<float>::max();

//         // Project hull onto axis
//         for (const auto& face : this->currentFaces) {
//             for (int i = 0; i < 3; i++) {
//                 float proj = glm::dot(face->vertices[i], axis);
//                 // std::cout << "Hull point projection: " << proj << std::endl;
//                 min1 = glm::min(min1, proj);
//                 max1 = glm::max(max1, proj);
//             }
//         }
//         // std::cout << "Hull projection range: " << min1 << " to " << max1 << std::endl;

//         // Project triangle onto axis
//         for (const auto& vertex : triangle->vertices) {
//             float proj = glm::dot(vertex, axis);
//             // std::cout << "Triangle vertex projection: " << proj << std::endl;
//             min2 = glm::min(min2, proj);
//             max2 = glm::max(max2, proj);
//         }
//         // std::cout << "Triangle projection range: " << min2 << " to " << max2 << std::endl;

//         // Check for overlap
//         return !(max1 < min2 || max2 < min1);
//     };

//     // Test face normals of the hull
//     for (const auto& face : this->currentFaces) {
//         // std::cout << "Testing hull face normal: " << face->normal.x << ", " << face->normal.y
//         <<
//         // ", " << face->normal.z << std::endl;
//         if (!test_axis(face->normal)) {
//             // std::cout << "found a non-intersecting seperating axis on face normal" <<
//             std::endl; return result;
//         }
//     }

//     // Test normal of the triangle
//     // std::cout << "Testing triangle normal: " << triangle->normal.x << ", " <<
//     triangle->normal.y
//     // << ", " << triangle->normal.z << std::endl;
//     if (!test_axis(triangle->normal)) {
//         // std::cout << "found a non-intersecting seperating axis on triangle normal" <<
//         std::endl; return result;
//     }

//     // Test cross product of edges
//     for (const auto& face : this->currentFaces) {
//         for (int i = 0; i < 3; i++) {
//             glm::vec3 edge1 = face->vertices[(i + 1) % 3] - face->vertices[i];
//             // std::cout << "Hull edge: " << edge1.x << ", " << edge1.y << ", " << edge1.z <<
//             // std::endl;
//             for (const auto& edge2 : {triangle->vertices[1] - triangle->vertices[0],
//                                       triangle->vertices[2] - triangle->vertices[1],
//                                       triangle->vertices[0] - triangle->vertices[2]}) {
//                 // std::cout << "Triangle edge: " << edge2.x << ", " << edge2.y << ", " <<
//                 edge2.z
//                 // << std::endl;
//                 glm::vec3 axis = glm::cross(edge1, edge2);
//                 // std::cout << "Cross product axis: " << axis.x << ", " << axis.y << ", " <<
//                 axis.z
//                 // << std::endl;
//                 if (glm::length2(axis) > 0.0001f && !test_axis(glm::normalize(axis))) {
//                     // std::cout << "found a non-intersecting seperating axis on edges" <<
//                     // std::endl;
//                     return result;
//                 }
//             }
//         }
//     }

//     // No separating axis found, shapes intersect
//     // std::cout << "No separating axis found. Shapes intersect." << std::endl;
//     result->collided = true;
//     return result;
// }

// CollisionResultPtr ConvexHull::intersectRay(const RayData& ray) const {
//     CollisionResultPtr result = new CollisionResult();
//     // std::cout << "testing ray against convex hull" << std::endl;
//     for (const auto& face : this->currentFaces) {
//         result = ray->intersectAABB(face->min, face->max);
//         if (result->collided) {
//             std::cout << "found a broad phase collision with ray and triangle" << std::endl;
//             result = face->intersectRay(ray);
//             if (result->collided) {
//                 std::cout << "found ray intersection" << std::endl;
//                 return result;
//             } else {
//                 std::cout << "failed narrow phase" << std::endl;
//             }
//         } else {
//             std::cout << "failed broad phase" << std::endl;
//         }
//     }

//     return result;
// }

// CollisionResultPtr ConvexHull::collideWithOBB(const OBBData& obb) const {
//     // Implementation
//     return new CollisionResult();
// }

// CollisionResultPtr ConvexHull::collideWithSphere(const Sphere* sphere) const {
//     // Implementation
//     return new CollisionResult();
// }

// CollisionResultPtr ConvexHull::collideWithConvexHull(const ConvexHullData& hull) const {
//     // Implementation
//     return new CollisionResult();
// }

// void ConvexHull::populateContactManifold(const CollisionShape* other, ContactManifold& manifold,
//                                          const CollisionResultPtr result,
//                                          ContactResolver& contactResolver) const {
//     // return this.populateContactManifoldWithConvexHull(*this);
// }

// void ConvexHull::populateContactManifoldWithAABB(const AABBData& aabb, ContactManifold& manifold,
//                                                  const CollisionResultPtr result,
//                                                  ContactResolver& contactResolver) const {
//     // 1. Check vertex-face contacts
//     float penetrationDepth = 0.0f;
//     for (const auto& face : this->currentFaces) {
//         for (const auto& vertex : face->vertices) {
//             if (std::abs(glm::dot(result->contactPoint.normal, face->normal)) > 0.707f) {
//                 if (AABBGeometry::containsPoint(aabb, vertex)) {
//                     penetrationDepth =
//                         ConvexHullContactDetails::calculatePenetrationDepthAABB(this, aabb);
//                     contactResolver.addContactPoint(manifold, vertex,
//                     result->contactPoint.normal,
//                                                     penetrationDepth, this, aabb);
//                 }
//             }
//         }
//     }

//     // // 2. Check edge-edge contacts
//     // for (const auto& edge : m_edges) {
//     //     for (const auto& obbEdge : OBB::edgeIndices) {
//     //         glm::vec3 point;
//     //         glm::vec3 obbCorner1 = obb.currentCorners[obbEdge.first];
//     //         glm::vec3 obbCorner2 = obb.currentCorners[obbEdge.second];
//     //         if (edgeEdgeIntersection(edge.first, edge.second, obbCorner1, obbCorner2, point))
//     {
//     //             contactResolver.addContactPoint(manifold, point,
//     //             calculateEdgeEdgeNormal(edge.first, edge.second, obbCorner1, obbCorner2),
//     //                             calculateEdgeEdgePenetrationDepth(point, edge.first,
//     edge.second,
//     //                             obbCorner1, obbCorner2), this, &obb);
//     //         }
//     //     }
//     // }
//     // 3. Check face-face contacts
//     for (const auto& face : this->currentFaces) {
//         glm::vec3 point;
//         if (ConvexHullContactDetails::faceAABBIntersection(face, aabb)) {
//             penetrationDepth = ConvexHullContactDetails::calculatePenetrationDepthAABB(this,
//             aabb); contactResolver.addContactPoint(manifold, point, face->normal,
//             penetrationDepth, this,
//                                             aabb);
//         }
//     }
// }

// void ConvexHull::populateContactManifoldWithPlane(const PlaneData& plane, ContactManifold&
// manifold,
//                                                   const CollisionResultPtr result,
//                                                   ContactResolver& contactResolver) const {
//     for (const auto& face : this->currentFaces) {
//         // might need to negate the dot product here
//         if (std::abs(glm::dot(result->contactPoint.normal, face->normal)) >
//             0.707f) {  // exclude this->currentFaces that are more than 45 degrees away from
//                        // parallel with contact normal
//             for (int i = 0; i < 3; i++) {
//                 float distance = glm::dot(plane->normal, face->vertices[i]) - plane->offset;
//                 if (distance < Config::EPSILON && distance > -Config::EPSILON) {
//                     contactResolver.addContactPoint(manifold, face->vertices[i], plane->normal,
//                                                     0.0f, this, plane);
//                 }
//                 if (distance < Config::EPSILON) {
//                     contactResolver.addContactPoint(
//                         manifold, face->vertices[i], plane->normal, -distance, this,
//                         plane);  // this should make penetrationDepth positive
//                 }
//             }
//         }
//     }
// }

// void ConvexHull::populateContactManifoldWithConvexHull(const ConvexHull& convexHull,
//                                                        ContactManifold& manifold,
//                                                        const CollisionResultPtr result,
//                                                        ContactResolver& contactResolver) const {
//     // Implementation
// }
