#pragma once
#include "core/PrecompiledHeader.hpp"
#include "rendering/InstancedMeshVertex.hpp"

class ShaderProgram;
class InstancedModel;

enum class ParticleType { RAIN, SNOW };

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float lifetime;
    float size;
};

class ParticleSystem {
   public:
    std::unique_ptr<InstancedModel> model;
    ParticleSystem();
    ~ParticleSystem() = default;
    ParticleType currentType = ParticleType::RAIN;
    void update(double deltaTime);
    void emit(unsigned int count);
    void setParticleLifetime(float minLifetime, float maxLifetime);
    void setParticleSize(float minSize, float maxSize);
    void setParticleColor(const glm::vec4& startColor, const glm::vec4& endColor);
    void updateBufferData();
    InstancedMeshVertex particleToInstancedVertex(const Particle& p);
    InstancedModel* getModel();

   private:
    std::vector<Particle> particles;
    unsigned int maxParticles;
    std::vector<InstancedMeshVertex> bufferData;
    glm::vec3 gravity;
    float minLifetime, maxLifetime;
    float minSize, maxSize;
    glm::vec4 startColor, endColor;
    float terminalVelocity;

    Particle createParticle();
};
