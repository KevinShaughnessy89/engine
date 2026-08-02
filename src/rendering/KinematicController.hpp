#pragma once

#include "entt-main/src/entt/entt.hpp"

struct KinematicBody;

// Runs gravity, grounding, and collision resolution for every KinematicBody in the registry.
// Movement input (deltaPos) and facing (yaw) are written by MovementController.
class KinematicController {
   public:
    KinematicController();
    ~KinematicController();

    void update(double deltaTime);

   private:
    void updateBody(entt::entity entity, double deltaTime);
};
