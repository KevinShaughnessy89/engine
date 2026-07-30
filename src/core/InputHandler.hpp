#include "core/PrecompiledHeader.hpp"
#ifndef _INPUTHANDLER
#define _INPUTHANDLER


class KeyHandling;
class MouseHandling;
class KeyObserver;
class MouseObserver;

struct PairHash {
    size_t operator()(const std::pair<int, int>& p) const noexcept {
        // pack two 32-bit ints into one 64-bit value
        uint64_t key = (uint64_t)(uint32_t)p.first << 32 | (uint32_t)p.second;
        return (size_t)key;  // truncates on 32-bit size_t, keeps full on 64-bit
    }
};

class InputHandler {
   public:
    InputHandler();
    static void keyboard_callback_(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButton_callback_(GLFWwindow* window, int button, int action, int mods);
    static void mouseCursor_callback_(GLFWwindow* window, double xposIn, double yposIn);
    void keyboard_callback(int key, int scancode, int action, int mods);
    void mouseButton_callback(int button, int action, int mods);
    void mouseCursor_callback(double xposIn, double yposIn);
    void processContinuousKeyboardInput(double deltaTime);
    void addMouseObserver(MouseObserver* observer);
    void removeMouseObserver(MouseObserver* observer);
    void addKeyObserver(KeyObserver* observer);
    void removeKeyObserver(KeyObserver* observer);

   private:
    KeyHandling* m_KeyHandler;
    MouseHandling* m_MouseHandler;
    static constexpr auto debounceTime = std::chrono::milliseconds(50);
    std::unordered_map<std::pair<int, int>, std::chrono::steady_clock::time_point, PairHash>
        lastKeyTime;
};

#endif
