#pragma once

#include "core/FixedRateClock.hpp"
#include "core/KeyObserver.hpp"
#include "core/PrecompiledHeader.hpp"
#include "gui/DebugPanel.hpp"
#include "gui/MainMenu.hpp"

class Core;

#define glCheckError() glCheckError_(__FILE__, __LINE__)

class Game : public KeyObserver {
   private:
    GLFWwindow* window;
    Core* systemCore;

   public:
    Game();
    bool init();
    void prewarmComponentStorage();
    void run();
    void updateMainThread(double deltaTime, double currentTime);
    void updateWorldState(double deltaTime, double currentTime);
    void runMainMenu();
    GameState updateLoading();
    GameState updateMenu();
    void onKeyEvent(int key, int scancode, int action, int mods) override;
    void onContinuousInput(double currentTime, const KeyHandling& keyHandler) override;
    static void focus_callback(GLFWwindow* window, int focused);
    GLenum glCheckError_(const char* file, int line);
    static GameState state;
    static bool showDebugPanel;
    MainMenu mainMenu;
    DebugPanel debugPanel;
    // Static (Game is a singleton, one instance in main.cpp) so other systems -- e.g.
    // ChunkManager::regenerateTerrain(), which needs to pause these before tearing down data the
    // collision-thread callbacks read -- can reach them without threading a Game reference through.
    static FixedRateClock* clock120;
    static FixedRateClock* clock240;
    bool clocksStarted = false;
    bool isInitializationComplete = false;
    // Debug panel's Pause/Step controls. Pausing stops updateWorldState() (camera/chunks/
    // environment/sun) AND the physics/collision FixedRateClocks, so a broken frame stays fully
    // still to poke at in the other debug tabs; Display.render() keeps running every real frame
    // regardless, so the (frozen) scene and ImGui stay visible/interactive throughout.
    static bool paused;
    // Consumed once by updateMainThread() to advance updateWorldState() a single time while
    // paused; does not tick the fixed-rate physics/collision clocks, which stay stopped.
    static bool stepRequested;
    static void setPaused(bool shouldPause);
};