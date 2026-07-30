#pragma once
#include "core/PrecompiledHeader.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>



enum class GameState { ENTRY, INIT, MAIN_MENU, RUNNING, LOADING, QUIT };
class GLFWwindow;

class MainMenu {
   public:
    MainMenu() = default;
    GameState renderMainMenu();
    GLFWwindow* window;
    void runEntrySplash();
    GameState renderLoadingScreen(float loadingProgress);
    void init(GLFWwindow* window);
    void close();

   private:
};
