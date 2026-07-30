#pragma once
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/Renderer.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/OcclusionMapGenerator.hpp"


inline void toggleFullscreen(GLFWwindow* window) {
    static int windowedX, windowedY, windowedWidth, windowedHeight;
    static bool isFullscreen = false;

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);

    if (!isFullscreen) {
        // Save windowed mode position and size
        glfwGetWindowPos(window, &windowedX, &windowedY);
        glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
        // Switch to fullscreen
        glfwSetWindowMonitor(window, primary, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        // Switch back to windowed mode with previous size/position
        glfwSetWindowMonitor(window, NULL, windowedX, windowedY, windowedWidth, windowedHeight, 0);
    }

    isFullscreen = !isFullscreen;
}

inline void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Update engine-side sizes

    if (width == 0 || height == 0) return;

    Display.getRenderer().setWindowWidth(width);
    Display.getRenderer().setWindowHeight(height);
    Display.setAspectValues((float)width, (float)height);

    // Resize FBO textures
    Display.getRenderer().updateFramebufferWindowSize();
    OcclusionMapGenerator::updateFramebufferSize();

    // Update viewport (global state for the context)
    glViewport(0, 0, width, height);
}
