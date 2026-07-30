#pragma once

#include <array>

#include "core/PrecompiledHeader.hpp"

enum class RenderLayer : uint8_t {
    Skybox,
    Sun,
    Terrain,
    Objects,
    InstancedObjects,
    Weather,
    Projectiles,
    TreeImposters,
    Animated,
};

inline RenderLayer renderLayerFromString(const std::string& typeStr) {
    if (typeStr == "Skybox") return RenderLayer::Skybox;
    if (typeStr == "Sun") return RenderLayer::Sun;
    if (typeStr == "Terrain") return RenderLayer::Terrain;
    if (typeStr == "Objects") return RenderLayer::Objects;
    if (typeStr == "InstancedObjects") return RenderLayer::InstancedObjects;
    if (typeStr == "Weather") return RenderLayer::Weather;
    if (typeStr == "Projectiles") return RenderLayer::Projectiles;
    if (typeStr == "TreeImposters") return RenderLayer::TreeImposters;
    if (typeStr == "Animated") return RenderLayer::Animated;

    std::cout << "Cannot find render layer " << typeStr << std::endl;
    return RenderLayer::Objects;
}

inline constexpr std::array<RenderLayer, 9> allRenderLayers = {
    RenderLayer::Skybox,           RenderLayer::Sun,
    RenderLayer::Terrain,          RenderLayer::Objects,
    RenderLayer::InstancedObjects, RenderLayer::Weather,
    RenderLayer::Projectiles,      RenderLayer::TreeImposters,
    RenderLayer::Animated,
};

inline const char* renderLayerToString(RenderLayer layer) {
    switch (layer) {
        case RenderLayer::Skybox:
            return "Skybox";
        case RenderLayer::Sun:
            return "Sun";
        case RenderLayer::Terrain:
            return "Terrain";
        case RenderLayer::Objects:
            return "Objects";
        case RenderLayer::InstancedObjects:
            return "InstancedObjects";
        case RenderLayer::Weather:
            return "Weather";
        case RenderLayer::Projectiles:
            return "Projectiles";
        case RenderLayer::TreeImposters:
            return "TreeImposters";
        case RenderLayer::Animated:
            return "Animated";
    }
    return "Unknown";
}