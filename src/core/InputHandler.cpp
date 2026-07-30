#include "core/InputHandler.hpp"

#include "core/KeyHandling.hpp"
#include "core/MouseHandling.hpp"
#include "core/PrecompiledHeader.hpp"
#include "gui/ImGuiFrameScope.hpp"

InputHandler::InputHandler() {
    m_KeyHandler = new KeyHandling();
    m_MouseHandler = new MouseHandling();
}

void InputHandler::processContinuousKeyboardInput(double deltaTime) {
    m_KeyHandler->processContinousInput(deltaTime);
}

void InputHandler::keyboard_callback_(GLFWwindow* window, int key, int scancode, int action,
                                      int mods) {
    InputHandler* inputHandler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

    // ALWAYS send to ImGui first (mirrors mouseButton_callback_) -- GLFW only allows one key
    // callback per window, and this one replaces ImGui's own during init, so without this,
    // non-printable keys (backspace, delete, arrows, enter) never reach ImGui's text editing.
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    inputHandler->m_KeyHandler->keyboard_callback(key, scancode, action, mods);
}

void InputHandler::mouseButton_callback_(GLFWwindow* window, int button, int action, int mods) {
    InputHandler* inputHandler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

    // 1. ALWAYS send to ImGui first
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    // 2. Debug AFTER forwarding (not before gating)
    ImGuiIO& io = ImGui::GetIO();

    inputHandler->m_MouseHandler->mouseButton_callback(button, action, mods);
}

void InputHandler::mouseCursor_callback_(GLFWwindow* window, double xposIn, double yposIn) {
    InputHandler* inputHandler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplGlfw_CursorPosCallback(window, xposIn, yposIn);

    inputHandler->m_MouseHandler->mouseCursor_callback(xposIn, yposIn);
}

// void InputHandler::keyboard_callback(int key, int scancode, int action, int mods) {
//     auto now = std::chrono::steady_clock::now();

//     std::pair<int, int> keyPress = std::make_pair(key, action);

//     auto it = lastKeyTime.find(keyPress);
//     if (it != lastKeyTime.end() && now - it->second < debounceTime) {
//         return;
//     }

//     lastKeyTime[keyPress] = now;

//     m_KeyHandler->keyboard_callback(key, scancode, action, mods);
// }

// void InputHandler::mouseButton_callback(int button, int action, int mods) {
//     m_MouseHandler->mouseButton_callback(button, action, mods);
// }

// void InputHandler::mouseCursor_callback(double xposIn, double yposIn) {
//     m_MouseHandler->mouseCursor_callback(xposIn, yposIn);
// }

void InputHandler::addMouseObserver(MouseObserver* observer) {
    m_MouseHandler->addObserver(observer);
}

void InputHandler::removeMouseObserver(MouseObserver* observer) {
    m_MouseHandler->removeObserver(observer);
}

void InputHandler::addKeyObserver(KeyObserver* observer) {
    m_KeyHandler->addObserver(observer);
}

void InputHandler::removeKeyObserver(KeyObserver* observer) {
    m_KeyHandler->removeObserver(observer);
}
