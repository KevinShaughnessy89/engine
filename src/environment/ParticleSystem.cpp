#include "ParticleSystem.hpp"

#include <glm/gtc/random.hpp>
#include <tracy/Tracy.hpp>

#include "components/physics/KinematicBody.hpp"
#include "components/rendering/ViewState.hpp"
#include "components/rendering/InstancedRenderable.hpp"
#include "core/Config.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/InstancedModel.hpp"
#include "rendering/ShaderProgram.hpp"

ParticleSystem::ParticleSystem() {
    maxParticles = 2000;
    minLifetime = 3.0f;
    maxLifetime = 6.0f;
    minSize = 0.01f;
    maxSize = 0.02f;
    startColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    gravity = glm::vec3(0.0f, Config::GRAVITY, 0.0f);
    terminalVelocity = -9.81f;
}

InstancedModel* ParticleSystem::getModel() {
    if (model)
        return model.get();
    else {
        std::vector<MeshVertex> quadVertices = {
            {{-0.5f, 0.5f, 0.0f, 1.0f}, {0, 0, 0}, {0, 1}, {0, 0, 0}, {0, 0, 0}},   // top-left
            {{0.5f, 0.5f, 0.0f, 1.0f}, {0, 0, 0}, {1, 1}, {0, 0, 0}, {0, 0, 0}},    // top-right
            {{-0.5f, -0.5f, 0.0f, 1.0f}, {0, 0, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0}},  // bottom-left
            {{0.5f, -0.5f, 0.0f, 1.0f}, {0, 0, 0}, {1, 0}, {0, 0, 0}, {0, 0, 0}},   // bottom-right
        };

        std::vector<unsigned int> quadIndices = {
            0, 2, 3,  // first triangle (top-left, bottom-left, bottom-right)
            0, 3, 1   // second triangle (top-left, bottom-right, top-right)
        };

        auto modelPtr =
            std::make_unique<InstancedModel>(std::move(quadVertices), std::move(quadIndices));

        this->model = std::move(modelPtr);
        return model.get();
    }
}

Particle ParticleSystem::createParticle() {
    Particle p;

    auto& cameraBody = CameraController::getKinematicBody(CameraController::activeCamera);
    auto& cameraView = CameraController::getViewState(CameraController::activeCamera);
    // Generate a random position relative to the camera
    float radius = 25.0f;  // Adjust this value to change the spawn area size
    float height = 10.0f;  // Height above the camera

    // Generate random angles
    float theta = glm::radians(glm::linearRand(0.0f, 360.0f));
    float phi = glm::radians(glm::linearRand(0.0f, 360.0f));

    // Calculate offset from camera
    glm::vec3 offset(radius * sin(phi) * cos(theta), height, radius * sin(phi) * sin(theta));

    // Transform offset to world space
    glm::vec3 right =
        glm::normalize(glm::cross(cameraView.forward, cameraView.forward));
    glm::mat3 cameraSpace(right, cameraView.up, -cameraView.forward);
    offset = cameraSpace * offset;
    // Set particle position
    p.position = cameraBody.position + offset;
    // Set a very small downward velocity
    p.velocity = glm::vec3(glm::linearRand(0.5f, 1.0f), -0.01f, glm::linearRand(0.5f, 1.0f));

    p.lifetime = glm::linearRand(minLifetime, maxLifetime);
    p.size = glm::linearRand(minSize, maxSize);
    p.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    return p;
}

void ParticleSystem::emit(unsigned int count) {
    for (unsigned int i = 0; i < count && particles.size() < maxParticles; ++i) {
        particles.push_back(createParticle());
    }
}

void ParticleSystem::update(double deltaTime) {
    ZoneScoped;
    ZoneValue(particles.size());
    for (auto it = particles.begin(); it != particles.end();) {
        Particle& p = *it;

        // Apply very slow downward movement
        p.position.x += p.velocity.x * deltaTime;
        p.position.y += terminalVelocity * deltaTime;
        p.position.z += p.velocity.z * deltaTime;

        p.lifetime -= deltaTime;

        if (p.lifetime <= 0) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }

    emit(1000);
    updateBufferData();
}

InstancedMeshVertex ParticleSystem::particleToInstancedVertex(const Particle& p) {
    InstancedMeshVertex vertex;
    vertex.position = p.position;
    vertex.size = glm::vec3(p.size);  // replicate the float size into a vec3
    vertex.color = glm::vec3(p.color.r, p.color.g, p.color.b);
    vertex.skew = currentType == ParticleType::RAIN ? glm::vec2(0.5f, 12.5f) : glm::vec2(1.f, 1.f);
    vertex.rotation = currentType == ParticleType::RAIN ? ((rand() % 20) - 10) * 0.0174533f
                                                        : ((rand() % 360) * 0.0174533f);
    return vertex;
}

void ParticleSystem::updateBufferData() {
    ZoneScoped;
    bufferData.clear();
    bufferData.reserve(particles.size());
    for (const auto& p : particles) {
        bufferData.push_back(particleToInstancedVertex(p));
    }
    model->updateBufferData(std::move(bufferData));
}

void ParticleSystem::setParticleLifetime(float minLifetime, float maxLifetime) {
    this->minLifetime = minLifetime;
    this->maxLifetime = maxLifetime;
}

void ParticleSystem::setParticleSize(float minSize, float maxSize) {
    this->minSize = minSize;
    this->maxSize = maxSize;
}
void ParticleSystem::setParticleColor(const glm::vec4& startColor, const glm::vec4& endColor) {
    this->startColor = startColor;
    this->endColor = endColor;
}
