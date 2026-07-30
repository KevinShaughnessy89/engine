#pragma once

class ParticleSystem;

class EnvironmentManager {
   public:
    EnvironmentManager();
    ~EnvironmentManager();
    void init();
    void update(double deltaTime);

   private:
    ParticleSystem* particleSystem;
};