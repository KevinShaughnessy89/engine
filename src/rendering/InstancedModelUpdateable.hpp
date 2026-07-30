#pragma once
#include <functional>

#include "UniformData.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/InstancedModel.hpp"


class ShaderProgram;
class InstancedModel;

class InstancedModelUpdateable {
   public:
    InstancedModel* model;
    std::vector<InstancedMeshVertex> bufferData;
    InstancedModelUpdateable() = default;
    bool shouldRender = false;
    std::vector<UniformData> latestUniforms;
    void setModel(InstancedModel* model) { this->model = model; };
    virtual void createModel() = 0;
    virtual void render(ShaderProgram* shader, int firstFreeTextureID) {
        this->model->render(shader, firstFreeTextureID, bufferData);
    }
    virtual std::vector<UniformData>& updateModelUniforms() = 0;
};
