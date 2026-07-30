#include "collision/shapes/OBB.hpp"

#include "collision/ContactManifold.hpp"
#include "collision/ContactResolver.hpp"
#include "collision/shapes/AABB.hpp"
#include "collision/shapes/ConvexHull.hpp"
#include "collision/shapes/Plane.hpp"
#include "collision/shapes/Ray.hpp"
#include "collision/shapes/Triangle.hpp"
#include "collision/shapes/contact/OBBContactDetails.hpp"
#include "collision/shapes/geometry/BoxGeometry.hpp"
#include "collision/shapes/geometry/OBBGeometry.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/Model.hpp"

//     6----7
//    /|   /|
//   2----3 |
//   | 4--|-5
//   |/   |/
//   0----1

void OBB::defineBoundingVolume(OBBData& obb, const GeometricData& geoData) {
    obb.max = geoData.max;
    obb.min = geoData.min;

    obb.initialAxes = {
        glm::normalize(glm::vec3(obb.max.x - obb.min.x, 0.0f, 0.0f)),
        glm::normalize(glm::vec3(0.0f, obb.max.y - obb.min.y, 0.0f)),
        glm::normalize(glm::vec3(0.0f, 0.0f, obb.max.z - obb.min.z)),
    };

    obb.initialAxes[1] = glm::cross(obb.initialAxes[2], obb.initialAxes[0]);

    for (auto& axis : obb.initialAxes) {
        axis = glm::normalize(axis);
    }

    float rangeX = obb.max.x - obb.min.x;
    float rangeY = obb.max.y - obb.min.y;
    float rangeZ = obb.max.z - obb.min.z;

    glm::vec3 initialCenter =
        glm::vec3((obb.max.x + obb.min.x) / 2.0f, (obb.max.y + obb.min.y) / 2.0f,
                  (obb.max.z + obb.min.z) / 2.0f);

    obb.currentCenter = initialCenter;
    // obb.initialCenter = initialCenter; -> why was this included, bug fix?

    obb.halfExtents = glm::vec3(rangeX / 2.0f, rangeY / 2.0f, rangeZ / 2.0f);

    obb.initialCorners.resize(8);
    obb.initialCorners[0] = glm::vec3(-obb.halfExtents.x, -obb.halfExtents.y, -obb.halfExtents.z);
    obb.initialCorners[1] = glm::vec3(obb.halfExtents.x, -obb.halfExtents.y, -obb.halfExtents.z);
    obb.initialCorners[2] = glm::vec3(-obb.halfExtents.x, obb.halfExtents.y, -obb.halfExtents.z);
    obb.initialCorners[3] = glm::vec3(obb.halfExtents.x, obb.halfExtents.y, -obb.halfExtents.z);
    obb.initialCorners[4] = glm::vec3(-obb.halfExtents.x, -obb.halfExtents.y, obb.halfExtents.z);
    obb.initialCorners[5] = glm::vec3(obb.halfExtents.x, -obb.halfExtents.y, obb.halfExtents.z);
    obb.initialCorners[6] = glm::vec3(-obb.halfExtents.x, obb.halfExtents.y, obb.halfExtents.z);
    obb.initialCorners[7] = glm::vec3(obb.halfExtents.x, obb.halfExtents.y, obb.halfExtents.z);

    obb.currentCorners = obb.initialCorners;
}

void OBB::update(OBBData& obb, const Position& position, const Orientation& orientation) {
    obb.previousCenter = obb.currentCenter;

    // initialCorners/initialCenter are in local (assimp) space; no scale is
    // ever applied, so translate-by-position * rotate is the full
    // local-to-world transform.
    obb.currentCorners = obb.initialCorners;
    for (auto& corner : obb.currentCorners) {
        corner = position.current + orientation.rotationMatrix * corner;
    }

    obb.currentCenter = position.current + orientation.rotationMatrix * obb.initialCenter;

    obb.min = obb.currentCenter - obb.halfExtents;
    obb.max = obb.currentCenter + obb.halfExtents;

    obb.currentAxes[0] = orientation.rotationMatrix[0];
    obb.currentAxes[1] = orientation.rotationMatrix[1];
    obb.currentAxes[2] = orientation.rotationMatrix[2];
}

CollisionResult OBB::collideWithPlane(const OBBData& obb, const PlaneData& planeData) {
    CollisionResult result;

    float rProj = std::abs(planeData.normal.x) * obb.halfExtents.x +
                  std::abs(planeData.normal.y) * obb.halfExtents.y +
                  std::abs(planeData.normal.z) * obb.halfExtents.z;

    float distance = glm::dot(planeData.normal, obb.currentCenter) - planeData.offset;

    result.collided = std::abs(distance) <= rProj;
    return result;
}

// CollisionResult OBB::collideWithAABB(const OBBData& obb, const AABBData& aabbData) {
//     CollisionResult result;

//     if (std::abs(aabbData.currentCenter.x - obb.currentCenter.x) >
//         (aabbData.halfExtents.x + obb.halfExtents.x))
//         return result;

//     if (std::abs(aabbData.currentCenter.y - obb.currentCenter.y) >
//         (aabbData.halfExtents.y + obb.halfExtents.y))
//         return result;

//     if (std::abs(aabbData.currentCenter.z - obb.currentCenter.z) >
//         (aabbData.halfExtents.z + obb.halfExtents.z))
//         return result;

//     result.collided = true;
//     return result;
// }

CollisionResult OBB::collideWithTriangle(const OBBData& obb, const TriangleData& tri) {
    CollisionResult result;

    std::vector<glm::vec3> axes;

    std::array<glm::vec3, 3> obbAxes = {
        glm::normalize(obb.currentCorners[1] - obb.currentCorners[0]),
        glm::normalize(obb.currentCorners[2] - obb.currentCorners[0]),
        glm::normalize(obb.currentCorners[4] - obb.currentCorners[0])};

    axes.insert(axes.end(), obbAxes.begin(), obbAxes.end());
    axes.push_back(tri.normal);

    std::array<glm::vec3, 3> triEdges = {tri.vertices[1] - tri.vertices[0],
                                         tri.vertices[2] - tri.vertices[1],
                                         tri.vertices[0] - tri.vertices[2]};

    for (const auto& obbAxis : obbAxes) {
        for (const auto& triEdge : triEdges) {
            glm::vec3 crossProduct = glm::cross(obbAxis, triEdge);
            float length = glm::length(crossProduct);
            if (length > 1e-6f) {
                axes.push_back(crossProduct / length);
            }
        }
    }

    for (const auto& axis : axes) {
        if (!OBBGeometry::testAxisSeparation(obb, axis,
                                             {tri.vertices[0], tri.vertices[1], tri.vertices[2]})) {
            return result;
        }
    }

    result.collided = true;
    return result;
}

CollisionResult OBB::intersectRay(const OBBData& obb, const RayData& rayData) {
    CollisionResult result;

    float tmin = 0.0f;
    float tmax = 100.0f;

    for (int i = 0; i < 3; i++) {
        if (std::abs(rayData.direction[i]) < Config::EPSILON) {
            if (rayData.origin[i] < obb.min[i] || rayData.origin[i] > obb.max[i]) return result;
        }

        float ood = 1.0f / rayData.direction[i];
        float t1 = (obb.min[i] - rayData.origin[i]) * ood;
        float t2 = (obb.max[i] - rayData.origin[i]) * ood;

        if (t1 > t2) std::swap(t1, t2);

        if (t1 > tmin) tmin = t1;
        if (t2 < tmax) tmax = t2;

        if (tmin > tmax) return result;
    }

    result.collided = true;
    result.contactPoint.position = rayData.origin + rayData.direction * tmin;
    return result;
}

void OBB::populateContactManifoldWithPlane(const OBBData& obb, const PlaneData& plane,
                                           ContactManifold& manifold,
                                           const CollisionResult& result) {
    int maxContact = 4;

    for (const auto& vertex : obb.currentCorners) {
        if (maxContact > 0) {
            float distance = glm::dot(plane.normal, vertex) - plane.offset;

            if (glm::epsilonEqual(distance, 0.0f, Config::EPSILON)) {
                manifold.addContactPoint(vertex, plane.normal, 0.0f);
                maxContact--;
            } else if (distance < 0) {
                manifold.addContactPoint(vertex, plane.normal, -distance);
                maxContact--;
            }
        }
    }
}

void OBB::populateContactManifoldWithConvexHull(const ShapeData& obb, const ShapeData& convexHull,
                                                ContactManifold& manifold,
                                                const CollisionResult& result) {
    // Implementation
}

void OBB::populateContactManifoldWithTriangle(const OBBData& obb, const TriangleData& triangle,
                                              ContactManifold& manifold,
                                              const CollisionResult& result) {
    std::cout << "Populating contact manifold for OBB-Triangle collision" << std::endl;

    glm::vec3 edge1 = triangle.vertices[1] - triangle.vertices[0];
    glm::vec3 edge2 = triangle.vertices[2] - triangle.vertices[0];
    glm::vec3 triangleNormal = glm::normalize(glm::cross(edge1, edge2));

    std::vector<glm::vec3> testAxes;

    testAxes.push_back(obb.currentAxes[0]);
    testAxes.push_back(obb.currentAxes[1]);
    testAxes.push_back(obb.currentAxes[2]);
    testAxes.push_back(triangleNormal);

    glm::vec3 triangleEdges[3] = {edge1, edge2, triangle.vertices[0] - triangle.vertices[2]};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            glm::vec3 crossProduct = glm::cross(obb.currentAxes[i], triangleEdges[j]);
            if (glm::length(crossProduct) > 1e-6f) {
                testAxes.push_back(glm::normalize(crossProduct));
            }
        }
    }

    float minOverlap = std::numeric_limits<float>::max();
    glm::vec3 separatingAxis;
    bool separated = false;

    for (const glm::vec3& axis : testAxes) {
        float obbMin, obbMax, triMin, triMax, overlap;

        BoxGeometry::projectOBBOntoAxis(obb, axis, obbMin, obbMax);
        BoxGeometry::projectTriangleOntoAxis(triangle.vertices[0], triangle.vertices[1],
                                             triangle.vertices[2], axis, triMin, triMax);

        if (!BoxGeometry::projectionsOverlap(obbMin, obbMax, triMin, triMax, overlap)) {
            separated = true;
            break;
        }

        if (overlap < minOverlap) {
            minOverlap = overlap;
            separatingAxis = axis;

            glm::vec3 triangleCenter =
                (triangle.vertices[0] + triangle.vertices[1] + triangle.vertices[2]) / 3.0f;
            if (glm::dot(separatingAxis, triangleCenter - obb.currentCenter) < 0) {
                separatingAxis = -separatingAxis;
            }
        }
    }

    if (separated) return;

    // 1. Triangle vertices inside OBB
    for (int i = 0; i < 3; i++) {
        if (BoxGeometry::isPointInside(obb, triangle.vertices[i])) {
            glm::vec3 normal;
            float penetration;
            BoxGeometry::getPointPenetration(obb, triangle.vertices[i], normal, penetration);
            std::cout << "added triangle vertex to contact manifold" << std::endl;
            manifold.addContactPoint(triangle.vertices[i], normal, penetration);
        }
    }

    // 2. OBB vertices inside triangle
    for (int i = 0; i < 8; i++) {
        glm::vec3 v = obb.currentCorners[i];
        glm::vec3 v0v1 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 v0v2 = triangle.vertices[2] - triangle.vertices[0];
        glm::vec3 v0v = v - triangle.vertices[0];

        float dot00 = glm::dot(v0v2, v0v2);
        float dot01 = glm::dot(v0v2, v0v1);
        float dot02 = glm::dot(v0v2, v0v);
        float dot11 = glm::dot(v0v1, v0v1);
        float dot12 = glm::dot(v0v1, v0v);

        float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
        float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
        float w = (dot00 * dot12 - dot01 * dot02) * invDenom;

        if (u >= 0 && w >= 0 && u + w <= 1) {
            std::cout << "barycentric test passed" << std::endl;
            float distanceToPlane = glm::dot(v - triangle.vertices[0], triangleNormal);
            if (std::abs(distanceToPlane) < minOverlap) {
                std::cout << "Added OBB vertex to contact manifold" << std::endl;
                manifold.addContactPoint(v, triangleNormal, std::abs(distanceToPlane));
            }
        }
    }

    // 3. Edge-edge fallback
    if (manifold.contacts.size() < 2) {
        glm::vec3 closestOnTriangle = triangle.vertices[0];
        float minDistSq = glm::dot(triangle.vertices[0] - obb.currentCenter,
                                   triangle.vertices[0] - obb.currentCenter);

        float dist1 = glm::dot(triangle.vertices[1] - obb.currentCenter,
                               triangle.vertices[1] - obb.currentCenter);
        float dist2 = glm::dot(triangle.vertices[2] - obb.currentCenter,
                               triangle.vertices[2] - obb.currentCenter);

        if (dist1 < minDistSq) {
            closestOnTriangle = triangle.vertices[1];
            minDistSq = dist1;
        }
        if (dist2 < minDistSq) {
            closestOnTriangle = triangle.vertices[2];
            minDistSq = dist2;
        }

        glm::vec3 edges[3] = {triangle.vertices[1] - triangle.vertices[0],
                              triangle.vertices[2] - triangle.vertices[1],
                              triangle.vertices[0] - triangle.vertices[2]};
        glm::vec3 edgeStarts[3] = {triangle.vertices[0], triangle.vertices[1],
                                   triangle.vertices[2]};

        for (int i = 0; i < 3; i++) {
            float t = glm::dot(obb.currentCenter - edgeStarts[i], edges[i]) /
                      glm::dot(edges[i], edges[i]);
            t = std::max(0.0f, std::min(1.0f, t));
            glm::vec3 pointOnEdge = edgeStarts[i] + edges[i] * t;
            float distSq =
                glm::dot(pointOnEdge - obb.currentCenter, pointOnEdge - obb.currentCenter);
            if (distSq < minDistSq) {
                closestOnTriangle = pointOnEdge;
                minDistSq = distSq;
            }
        }

        glm::vec3 closestOnOBB = BoxGeometry::getClosestPointOnOBB(obb, closestOnTriangle);
        glm::vec3 contactVector = closestOnTriangle - closestOnOBB;
        float contactDistance = glm::length(contactVector);

        if (contactDistance > 1e-6f && contactDistance < minOverlap && manifold.contacts.empty()) {
            std::cout << "Added edge-edge contact to manifold" << std::endl;
            manifold.addContactPoint((closestOnOBB + closestOnTriangle) * 0.5f,
                                     glm::normalize(contactVector), contactDistance);
        }
    }

    if (manifold.contacts.size() > 4) {
        std::sort(manifold.contacts.begin(), manifold.contacts.end(),
                  [](const ContactPoint a, const ContactPoint b) {
                      return a.penetrationDepth > b.penetrationDepth;
                  });
        manifold.contacts.resize(4);
    }
}

void OBB::populateContactManifoldWithSphere(const OBBData& obb, const SphereData& sphere,
                                            ContactManifold& manifold,
                                            const CollisionResult& result) {
    // Implementation
}

glm::vec3 OBB::calculateContactNormal(const OBBData& obb, const glm::vec3& intersectionPoint) {
    return glm::vec3(0.0f);
}

float OBB::getExtentAlongDirection(const OBBData& obb, const glm::vec3& direction) {
    float min, max;
    BoxGeometry::projectOBBOntoAxis(obb, direction, min, max);
    return (max - min) * 0.5f;
}

std::vector<int> OBB::getFurthestVertices(const OBBData& obb, const glm::vec3& point) {
    return std::vector<int>();
}

glm::vec3 OBB::getVertexAtIndex(const OBBData& obb, int index) {
    return glm::vec3(0.0f);
}

glm::vec3 OBB::getOppositeEdgeCenter(const OBBData& obb,
                                     const std::pair<glm::vec3, glm::vec3>& edge,
                                     const glm::vec3& normal) {
    return glm::vec3(0.0f);
}

CollisionResult OBB::collideWithOBB(const OBBData& obb, const OBBData& other) {
    return CollisionResult{};
}