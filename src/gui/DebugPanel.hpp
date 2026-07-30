#pragma once
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include "components/rendering/SunState.hpp"

class DebugPanel {
   public:
    DebugPanel() = default;
    void renderDebugPanel();
    // Draws whichever debug-panel toggles opt into the on-screen overlay (currently just the FPS
    // counter) as plain text over the viewport, independent of whether the panel itself is open.
    // Called every frame from Game::run(). Add future overlay-able stats here as more checkboxes
    // are added.
    void renderOverlay();
    void init();
    void close();

   private:
    // Shadow tab: edited by the slider, only applied to ShadowState::shadowCascadeCount (and the
    // shadow texture array reallocated) when "Apply" is pressed -- reallocating on every slider
    // drag tick would tear down/rebuild the GL resources dozens of times per second.
    int pendingCascadeCount = -1;
    // Terrain > Rendering sub-tab: same pending-value-until-Apply pattern as pendingCascadeCount,
    // but the regen this triggers (ChunkManager::regenerateTerrain()) is far more expensive, so
    // it's even more important not to fire it on every slider tick.
    int pendingTriangleCount = -1;
    // Sky tab: snapshot of SunState taken the first time the tab is opened (before any edits),
    // restored verbatim by the Revert button. DebugState::sunManualOverride freezes the per-frame
    // sun simulation from the moment this is taken, so it stays a meaningful baseline.
    SunState cachedSunState;
    bool sunStateCached = false;
};