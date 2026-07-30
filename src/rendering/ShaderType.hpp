#pragma once

#include "core/PrecompiledHeader.hpp"

enum class ShaderType {
    Model,
    AnimatedModel,
    Light,
    Terrain,
    Sky,
    Particle,
    Blur,
    Combine,
    Sun,
    NormalMap,
    ShadowDepth,
    InstancedShadowDepth,
    InstancedModel,
    ImposterModel
};

inline ShaderType shaderTypeFromString(const std::string& typeStr) {
    if (typeStr == "Model") return ShaderType::Model;
    if (typeStr == "AnimatedModel") return ShaderType::AnimatedModel;
    if (typeStr == "Light") return ShaderType::Light;
    if (typeStr == "Terrain") return ShaderType::Terrain;
    if (typeStr == "Sky") return ShaderType::Sky;
    if (typeStr == "Particle") return ShaderType::Particle;
    if (typeStr == "Blur") return ShaderType::Blur;
    if (typeStr == "Combine") return ShaderType::Combine;
    if (typeStr == "Sun") return ShaderType::Sun;
    if (typeStr == "NormalMap") return ShaderType::NormalMap;
    if (typeStr == "ShadowDepth") return ShaderType::ShadowDepth;
    if (typeStr == "InstancedModel") return ShaderType::InstancedModel;
    if (typeStr == "ImposterModel") return ShaderType::ImposterModel;

    std::cout << "Cannot find shader " << typeStr << std::endl;
    return ShaderType::Model;
}
