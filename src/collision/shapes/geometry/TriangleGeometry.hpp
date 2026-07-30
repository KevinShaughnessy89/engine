#pragma once
#include "core/PrecompiledHeader.hpp"

#include "collision/shapes/geometry/GeometryObject.hpp"
#include "components/collision/ShapeData.hpp"

class Ray;
class Triangle;

class TriangleGeometry : public GeometryObject {
   public:
    TriangleGeometry() {}
    static void calculateTriangleMinMax(TriangleData& triangle);
    static void calculateNormal(TriangleData& triangle);
    static float getDistanceAlongNormal(const TriangleData& triangle, const RayData& ray);
    static glm::vec3 closestPointOnTriangle(const TriangleData& triangle, const glm::vec3& point);
    static glm::vec3 closestPointOnEdge(const TriangleData& triangle, glm::vec3 point, int edge);
    static bool isPointOnTriangleFace(const TriangleData& triangle, const glm::vec3& point);
    static bool isPointOnTriangleEdge(const TriangleData& triangle, const glm::vec3& point);
    static bool isPointInTriangle(const TriangleData& triangle, const glm::vec3& point);
    static bool isPointInTriangleBarycentric(const TriangleData& triangle,
                                             const glm::vec3& barycentric);
    static bool checkVertexIntersection(const RayData& ray, const glm::vec3& vertex, float epsilon);
    static bool checkEdgeIntersection(const RayData& ray, const glm::vec3& v1, const glm::vec3& v2,
                                      float epsilon);
    static glm::vec3 worldToBarycentric(const TriangleData& triangle, const glm::vec3& worldPoint);
    static glm::vec3 barycentricToWorld(const TriangleData& triangle, const glm::vec3& barycentric);
    glm::vec3 getBarycentricCoordinates(const TriangleData& triangle, const glm::vec3& cameraPosition);
    static float getBarycentricHeightFromWorld(const TriangleData& triangle,
                                               const glm::vec3& worldPoint);
};
