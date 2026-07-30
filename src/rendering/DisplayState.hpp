#pragma once

#include "core/Config.hpp"

// Populated by DisplayManager (on construction and on window resize via setAspectValues), read by
// anything that needs the camera's perspective parameters without including the whole
// DisplayManager -- same role ShadowState plays for the shadow map data.
namespace DisplayState {
inline static GLFWwindow* window = nullptr;
inline float nearPlane = 1.0f;
inline float farPlane = 100000.0f;
inline float perspectiveFOV = 75.0f;
inline static float screenWidth = []() -> float {
    if (window) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        return static_cast<float>(width);
    }
    return static_cast<float>(Config::SCREEN_WIDTH);
}();
inline static float screenHeight = []() -> float {
    if (window) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        return static_cast<float>(height);
    }
    return static_cast<float>(Config::SCREEN_HEIGHT);
}();
inline float aspect = 0.0f;
inline glm::mat4 perspectiveMatrix = glm::mat4(1.0f);
}  // namespace DisplayState
