#include "InstancedModel.hpp"

#include <tracy/Tracy.hpp>

#include "components/collision/ShapeData.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/TextureManager.hpp"

InstancedModel::InstancedModel() {
}

InstancedModel::InstancedModel(std::vector<MeshVertex>&& vertices) {
    instancedMeshes.emplace_back(std::move(vertices));
}

InstancedModel::InstancedModel(std::vector<MeshVertex>&& vertices,
                               std::vector<unsigned int>&& indices) {
    instancedMeshes.emplace_back(std::move(vertices), std::move(indices));
}

InstancedModel::InstancedModel(std::vector<MeshVertex>&& vertices,
                               std::vector<unsigned int>&& indices, Texture* texture) {
    std::vector<Texture*> textureVec = {texture};
    instancedMeshes.emplace_back(std::move(vertices), std::move(indices), std::move(textureVec));
}

InstancedModel::InstancedModel(std::vector<MeshVertex>&& vertices,
                               std::vector<unsigned int>&& indices,
                               std::vector<Texture*>&& textures) {
    instancedMeshes.emplace_back(std::move(vertices), std::move(indices), std::move(textures));
}

void InstancedModel::render(ShaderProgram* shader, int firstFreeTextureID) {
    for (auto& mesh : instancedMeshes) {
        mesh.render(shader, firstFreeTextureID);
    }
}

glm::vec3 InstancedModel::getMaxCoordinates() {
    return glm::vec3(max[0], max[1], max[2]);
}

glm::vec3 InstancedModel::getMinCoordinates() {
    return glm::vec3(min[0], min[1], min[2]);
}

GeometricData InstancedModel::calculateGeometricData() {
    GeometricData geoData;
    geoData.min = getMinCoordinates();
    geoData.max = getMaxCoordinates();
    return geoData;
}

GeometricData InstancedModel::calculateGeometricDataForMaterial(const std::string& material) {
    GeometricData geoData;
    geoData.min = glm::vec3(std::numeric_limits<float>::infinity());
    geoData.max = glm::vec3(-std::numeric_limits<float>::infinity());

    for (const auto& mesh : instancedMeshes) {
        if (mesh.material != material) continue;
        for (const auto& vertex : mesh.vertices) {
            geoData.min = glm::min(geoData.min, glm::vec3(vertex.position));
            geoData.max = glm::max(geoData.max, glm::vec3(vertex.position));
        }
    }

    return geoData;
}

void InstancedModel::updateBufferData(std::vector<InstancedMeshVertex>&& bufferData) {
    for (size_t i = 0; i < instancedMeshes.size(); i++) {
        if (i + 1 == instancedMeshes.size()) {
            instancedMeshes[i].instancedBufferData = std::move(bufferData);
        } else {
            instancedMeshes[i].instancedBufferData = bufferData;
        }
    }
}

void InstancedModel::setInstances(std::vector<InstancedMeshVertex>&& instances) {
    allInstances = std::move(instances);
}

void InstancedModel::updateVisibleInstances(
    const std::vector<InstancedMeshVertex>& visibleInstances) {
    ZoneScoped;
    ZoneValue(visibleInstances.size());
    for (auto& mesh : instancedMeshes) {
        mesh.instancedBufferData = visibleInstances;
        mesh.uploadInstanceData();
    }
}

void InstancedModel::addTexture(std::string name, std::string uniform) {
    Texture* texture = Textures.getTexture(name);
    texture->setUniform(uniform);
    for (auto& mesh : instancedMeshes) {
        mesh.textures.push_back(texture);
    }
}
