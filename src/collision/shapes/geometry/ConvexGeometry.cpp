// #include "ConvexGeometry.hpp"

// #include "collision/shapes/ConvexHull.hpp"
// #include "collision/shapes/Triangle.hpp"
// #include "core/PrecompiledHeader.hpp"
// #include "rendering/Model.hpp"

// ConvexGeometry::ConvexGeometry() {
// }

// void ConvexGeometry::buildHull(ConvexHullData& hull) {
//     float threshold = 1.0f;
//     buildMeshComponents(hull);
//     createAdjacencyList(hull);
//     weldVerticesAndUpdateIndices(hull, threshold);
// }

// bool ConvexGeometry::buildMeshComponents(ConvexHullData& hull) {
//     assert(hull->indices.size() % 3 == 0);

//     for (int i = 0; i < hull->indices.size(); i += 3) {
//         TriangleData& face =
//             new Triangle(hull->vertices[hull->indices[i]], hull->vertices[hull->indices[i + 1]],
//                          hull->vertices[hull->indices[i + 2]], hull->indices[i],
//                          hull->indices[i + 1], hull->indices[i + 2]);
//         hull->initialFaces.push_back(face);
//     }
//     hull->currentFaces = hull->initialFaces;

//     createAdjacencyList(hull);

//     for (int i = 0; i < hull->vertices.size(); ++i) {
//         if (isCornerVertex(hull, i)) {
//             hull->cornerIndices.push_back(i);
//         }
//     }

//     return true;
// }

// bool ConvexGeometry::isCornerVertex(const ConvexHullData& hull, int vertexIndex) {
//     float totalSolidAngle = 0.0f;
//     for (int triangleIndex : hull->adjacencyList[vertexIndex]) {
//         float angle = calculateSolidAngle(hull, vertexIndex, hull->initialFaces[triangleIndex]);
//         totalSolidAngle += angle;
//         // std::cout << "  Triangle " << triangleIndex << ": Solid Angle = " << angle <<
//         std::endl; if (totalSolidAngle >= 1.9f * glm::pi<float>()) {
//             return false;
//         }
//     }
//     // std::cout << "Vertex " << vertexIndex << ": Total Solid Angle = " << totalSolidAngle <<
//     // std::endl;
//     return totalSolidAngle >= 0.1f * glm::pi<float>();
// }

// float ConvexGeometry::calculateSolidAngle(const ConvexHullData& hull, int vertexIndex,
//                                           const TriangleData& triangle) {
//     glm::vec3 v = hull->vertices[vertexIndex];

//     std::vector<glm::vec3> edges;
//     for (int i = 0; i < 3; ++i) {
//         if (triangle->indices[i] != vertexIndex) {
//             edges.push_back(glm::normalize(triangle->vertices[i] - v));
//         }
//     }

//     if (edges.size() != 2) {
//         // std::cout << "Error: Invalid number of edges for vertex " << vertexIndex << std::endl;
//         return 0.0f;
//     }

//     float cosA = glm::dot(edges[0], edges[1]);
//     float cosB = glm::dot(edges[0], triangle->normal);
//     float cosC = glm::dot(edges[1], triangle->normal);

//     // Clamp values to avoid domain errors in acos
//     cosA = glm::clamp(cosA, -1.0f, 1.0f);
//     cosB = glm::clamp(cosB, -1.0f, 1.0f);
//     cosC = glm::clamp(cosC, -1.0f, 1.0f);

//     float A = std::acos(cosA);
//     float B = std::acos(cosB);
//     float C = std::acos(cosC);

//     float solidAngle = A + B + C - glm::pi<float>();

//     // std::cout << "Vertex " << vertexIndex << " - Solid Angle: " << solidAngle
//     //           << " radians, " << glm::degrees(solidAngle) << " degrees" << std::endl;

//     return solidAngle;
// }

// void ConvexGeometry::createAdjacencyList(ConvexHullData& hull) {
//     hull->adjacencyList.resize(hull->initialFaces.size(), std::vector<int>(3));
//     for (int i = 0; i < hull->initialFaces.size(); i++) {
//         for (int j = 0; j < 3; j++) {
//             int vertexIndex = hull->initialFaces[i]->indices[j];
//             hull->adjacencyList[vertexIndex].push_back(i);
//         }
//     }
// }

// void ConvexGeometry::weldVerticesAndUpdateIndices(ConvexHullData& hull, float threshold) {
//     for (size_t i = 0; i < hull->vertices.size(); ++i) {
//         for (size_t j = i + 1; j < hull->vertices.size();) {
//             if (glm::distance(hull->vertices[i], hull->vertices[j]) <= threshold) {
//                 // Weld vertex j to vertex i

//                 // Update all hull->indices that referred to j
//                 for (int& index : hull->indices) {
//                     if (index == j) {
//                         index = i;
//                     } else if (index > j) {
//                         // Decrement hull->indices that come after j, as we're about to remove j
//                         --index;
//                     }
//                 }

//                 // Remove vertex j
//                 hull->vertices.erase(hull->vertices.begin() + j);

//                 // Don't increment j, as we've removed an element and the next one has shifted
//                 into
//                 // its place
//             } else {
//                 ++j;
//             }
//         }
//     }

//     // Remove degenerate triangles
//     for (size_t i = 0; i < hull->indices.size(); i += 3) {
//         if (hull->indices[i] == hull->indices[i + 1] ||
//             hull->indices[i + 1] == hull->indices[i + 2] ||
//             hull->indices[i + 2] == hull->indices[i]) {
//             hull->indices.erase(hull->indices.begin() + i, hull->indices.begin() + i + 3);
//             i -= 3;  // Adjust for the removed triangle
//         }
//     }
// }

// glm::vec3 ConvexGeometry::calculateEdgeEdgeNormal(const glm::vec3& edge1Start,
//                                                   const glm::vec3& edge1End,
//                                                   const glm::vec3& edge2Start,
//                                                   const glm::vec3& edge2End) {
//     glm::vec3 edge1Dir = glm::normalize(edge1End - edge1Start);
//     glm::vec3 edge2Dir = glm::normalize(edge2End - edge2Start);
//     return glm::normalize(glm::cross(edge1Dir, edge2Dir));
// }

// void ConvexGeometry::projectHullOntoAxis(const ConvexHullData& hull, const glm::vec3& axis,
//                                          float& min, float& max) {
//     for (size_t i = 1; i < hull->vertices.size(); ++i) {
//         float projection = glm::dot(hull->vertices[i], axis);
//         min = std::min(min, projection);
//         max = std::max(max, projection);
//     }
// }

// std::vector<glm::vec3> ConvexGeometry::getEdges(const ConvexHullData& hull) {
//     std::unordered_set<glm::vec3, Vector3Hash> edgeSet;

//     for (const auto& face : hull->currentFaces) {
//         edgeSet.emplace(glm::vec3(face->vertices[0] - face->vertices[1]));
//         edgeSet.emplace(glm::vec3(face->vertices[1] - face->vertices[2]));
//         edgeSet.emplace(glm::vec3(face->vertices[2] - face->vertices[0]));
//     }

//     return std::vector<glm::vec3>(edgeSet.begin(), edgeSet.end());
// }

// glm::vec3 ConvexGeometry::findNormalAtPoint(const ConvexHullData& hull, const glm::vec3& point) {
//     for (const auto& face : hull->currentFaces) {
//         if (TriangleGeometry::isPointInTriangle(face, point)) {
//             return face->normal;
//         }
//     }
// }
