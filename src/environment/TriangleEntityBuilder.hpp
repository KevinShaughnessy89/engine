#pragma once

#include "collision/shapes/Triangle.hpp"
#include "environment/TriangleManager.hpp"

class TriangleEntityBuilder {
   public:
    TriangleEntityBuilder() = default;

    std::array<glm::vec3, 3> vertices;
    std::array<unsigned int, 3> indices;

    void addData(glm::vec3 vertexA, glm::vec3 vertexB, glm::vec3 vertexC, unsigned int indexA,
                 unsigned int indexB, unsigned int indexC) {
        vertices[0] = vertexA;
        vertices[1] = vertexB;
        vertices[2] = vertexC;
        indices[0] = indexA;
        indices[1] = indexB;
        indices[2] = indexC;
    }

    uint32_t buildEntity(TriangleManager& triangleManager, size_t chunkSlot) const {
        TriangleData triangleData = TriangleData();
        triangleData.vertices[0] = vertices[0];
        triangleData.vertices[1] = vertices[1];
        triangleData.vertices[2] = vertices[2];
        triangleData.indices[0] = indices[0];
        triangleData.indices[1] = indices[1];
        triangleData.indices[2] = indices[2];
        Triangle::defineBoundingVolume(triangleData);
        return triangleManager.create(chunkSlot, std::move(triangleData));
    }
};