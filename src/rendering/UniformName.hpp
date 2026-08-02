#pragma once

// Fixed set of uniform names used by UniformUpdater. Kept as an enum (int) rather than
// std::string so per-uniform bookkeeping (UniformData) doesn't heap-allocate a string every
// frame; toCString() maps back to the literal GLSL name only at the point of the actual GL call.
enum class UniformName : int {
    Model,
    GlobalMinHeight,
    GlobalMaxHeight,
    WorldMin,
    WorldMax,
    ViewPos,
    SunPosition,
    LightColor,
    AmbientStrength,
    LightSpaceMatrix,
    SunColor,
    Intensity,
    LightPos,
    UseNormalMap,
    View,
    Projection,
    ShadowMapArray,
    SunTopColor,
    SunHorizonColor,
    SunBottomColor,
    SkyColorAverage,
    IrradianceMap,
    GridRes,
    TreeImposter,
    SSAOBlurTexture,
    ScreenWidth,
    ScreenHeight,
    UseAlphaMap,
    SkinSlot,
    Tint,
    Count
};

inline constexpr const char* UniformNameStrings[static_cast<int>(UniformName::Count)] = {
    "model",           "globalMinHeight", "globalMaxHeight", "worldMin",         "worldMax",
    "viewPos",         "sunPosition",     "lightColor",      "ambientStrength",  "lightSpaceMatrix",
    "sunColor",        "intensity",       "lightPos",        "useNormalMap",     "view",
    "projection",      "shadowMapArray",  "topColor",        "horizonColor",     "bottomColor",
    "skyColorAverage", "irradianceMap",   "gridRes",         "treeAtlasTexture", "ssaoBlurTexture",
    "screenWidth",     "screenHeight",    "useAlphaMap",     "skinSlot",         "tint"};

inline constexpr const char* toCString(UniformName name) {
    return UniformNameStrings[static_cast<int>(name)];
}

inline constexpr UniformName fromCString(const char* string) {
    UniformName name = UniformName::Count;  // default to invalid
    for (int i = 0; i < static_cast<int>(UniformName::Count); i++) {
        if (std::strcmp(string, UniformNameStrings[i]) == 0) {
            name = static_cast<UniformName>(i);
            break;
        }
    }

    return name;
}