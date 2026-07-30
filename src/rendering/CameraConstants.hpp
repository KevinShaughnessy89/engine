#pragma once

namespace CameraConstants {
const glm::vec3 WORLD_UP_AXIS = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 WORLD_FORWARD_AXIS = glm::vec3(0.0f, 0.0f, -1.0f);
const glm::vec3 WORLD_RIGHT_AXIS = glm::cross(WORLD_FORWARD_AXIS, WORLD_UP_AXIS);
inline float mouseSensitivity = 1.f;
inline float terminalVelocity = -56.f;
inline float gravity = -240.f;
inline float maxYawRate = 10.f;
inline float headHeight = 1.5f;
inline float maxPitchRate = 10.f;
inline float thirdPersonDistance = 5.f;  // how far behind the body the camera trails
inline float thirdPersonHeight = 2.f;    // how far above the body the camera sits
};  // namespace CameraConstants