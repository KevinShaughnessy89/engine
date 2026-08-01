#pragma once

#include "CameraConstants.hpp"

struct KinematicBody;

// Runs gravity, grounding, and collision resolution for every KinematicBody in the registry.
// Movement input (deltaPos) and facing (yaw) are written by MovementController.
class KinematicController {
   public:
    KinematicController();
    ~KinematicController();

    void update(double deltaTime);

   private:
    void updateBody(KinematicBody& body, double deltaTime);
};
