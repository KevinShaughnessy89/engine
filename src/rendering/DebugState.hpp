#pragma once

#include <unordered_map>

#include "environment/TerrainConfig.hpp"
#include "rendering/GLState.hpp"
#include "rendering/RenderLayer.hpp"

// Per-layer GL-state overrides driven by the ImGui debug panel (DebugPanel.cpp, "Rendering" tab).
// Each layer's entry starts equal to GLStateFlags::forLayer(that layer) -- the same baseline every
// Renderable/ChunkRenderable/InstancedRenderable in that layer already computes at construction --
// so nothing changes until a checkbox is actually flipped in the panel.
// GLState::applyState() substitutes these fields onto the entity's own required state,
// scoped to the entity's own layer, before diffing against GLState::current.
namespace DebugState {

struct LayerDebugState {
    bool wireframe = false;
    bool depthTest = true;
    bool depthMask = true;
    bool cullFace = true;
    bool blend = true;
    bool depthClamp = false;
};

// Backing state for the panel's "All" section -- editing a checkbox there stamps that field
// across every entry in layerStates (see DebugPanel::renderDebugPanel()). Not read by
// applyState() itself; it's only a write-through convenience on top of layerStates.
inline LayerDebugState allState;

// Stats tab: toggled by the "Overlay" checkbox, drawn by DebugPanel::renderOverlay() every frame
// (independent of the debug panel's own visibility). currentFPS is updated once per frame in
// Game::run().
inline bool showFpsOverlay = false;
inline float currentFPS = 0.0f;

// Sky tab: while true, Game::updateWorldState() skips SunHelper::updateSunState() so the
// per-frame sun simulation doesn't stomp the debug panel's manual SunState edits (and so
// DebugPanel's cached "original" values have something stable to Revert to). Set the first time
// the Sky tab caches its baseline.
inline bool sunManualOverride = false;

// Terrain tab: gates ChunkGenerationPipeline::runPostProcessing()'s call to
// TerrainAssetFactory::generateTreePlacements().
inline bool shouldGenerateFoliage = false;

// Terrain > Noise tab: domain-warp params for getHeightAt() (strength is a world-space offset).
inline float warpFrequency = 0.00001f;
inline float warpStrength = 3000.0f;

// Terrain > Noise tab: global noise frequencies. Spatially uniform on purpose -- a biome-blended
// frequency samples at x*f(x), and the x*grad(f) term shears ripples/needles at transitions.
inline float lowlandFrequency = 0.0002f;
inline float mountainFrequency = 0.0000001f;

// Terrain > Noise tab: moisture/temperature pseudoNoise frequencies and the Gaussian falloff
// (sigma) used to weight each biome's distance from getHeightAt()'s moisture/temperature sample --
// smaller sigma makes biome transitions sharper, larger blends them more gradually.
inline float moistureFrequency = 0.00005f;
inline float temperatureFrequency = 0.00025f;
inline float biomeSigma = 0.35f;

// Terrain > Noise tab: amplitude (in world units) below which blended macro amplitude goes to the
// lowland layer, with the remainder going to the mountain layer.
inline float biomeAmplitudeSplit = 80.0f;

// Terrain > Noise tab: exponent sharpening the raw mountain fBm (sign-preserving pow) -- higher
// values carve flatter valleys and sharper peaks.
inline float mountainSharpness = 4.0f;

// Terrain > Noise tab: smoothstep thresholds (on blended macro amplitude) controlling where the
// mountain-micro erosion detail fades in.
inline float mountainMaskThresholdLow = 80.5f;
inline float mountainMaskThresholdHigh = 1200.0f;

// Terrain > Noise tab: Worley/cellular massif detail layered on top of the mountain shape for fine
// peak roughness. Called directly with raw world coordinates (see HeightMapGenerator::getHeightAt)
// rather than through an fBm octave loop, since cellSize IS the frequency for cellular noise.
inline float mountainMicroCellSize = 400.0f;
inline float mountainMicroSmoothing = 1.0f;  // 0 = sharp Voronoi creases, 1 = fully smooth
inline float mountainMicroSharpness = 1.0f;  // 1 = linear falloff, >1 = sharper peak, <1 = flatter
inline float mountainDetailStrength = 0.04f;

// Terrain > Noise tab: second Worley/cellular detail signal, tracing the cell-boundary ridge
// network (F2-F1) instead of massifEnvelope's per-cell dome -- also called directly with raw world
// coordinates (see HeightMapGenerator::getHeightAt).
inline float mountainMicroRidgeCellSize = 400.0f;
inline float mountainMicroRidgeSharpness = 1.0f;
inline float mountainMicroRidgeStrength = 0.04f;

// Terrain > Noise tab: multiplies the mountain layer's contribution to the final height (h =
// lowland*lowlandAmplitude + mountain*mountainAmplitude*debugAmplitudeScale), independent of
// biome-blended amplitude.
inline float debugAmplitudeScale = 2.0f;

// Per-layer submitted/rendered entity counts, refilled from scratch each frame by
// Renderer::renderBatches() -- "submitted" is however many entities/slots/instances existed
// in that layer this frame, "rendered" is however many survived frustum/occlusion culling and
// actually issued a draw call. Read by DebugPanel's Render Stats tab; not meant for anything else
// to depend on since it's cleared and repopulated every frame.
struct LayerRenderStats {
    int submitted = 0;
    int rendered = 0;
};
inline std::unordered_map<RenderLayer, LayerRenderStats> layerRenderStats;

inline std::unordered_map<RenderLayer, LayerDebugState> layerStates = [] {
    std::unordered_map<RenderLayer, LayerDebugState> states;
    for (RenderLayer layer : allRenderLayers) {
        GLStateFlags defaults = GLStateFlags::forLayer(layer);
        LayerDebugState state;
        state.depthTest = defaults.depthTest;
        state.depthMask = defaults.depthMask;
        state.cullFace = defaults.cullFace;
        state.blend = defaults.blend;
        state.depthClamp = defaults.depthClamp;
        // wireframe has no entity-driven counterpart -- always starts off.
        states[layer] = state;
    }
    return states;
}();

}  // namespace DebugState
