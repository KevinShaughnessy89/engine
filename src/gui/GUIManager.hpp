#pragma once
#include "core/PrecompiledHeader.hpp"

class MainMenu;

class GUIManager {

    public:
        MainMenu* mainMenu;
        
        void init(GLFWwindow* window);
        void runMainMenu() {}
};
