#pragma once

#include "core/PrecompiledHeader.hpp"

enum class ModelType { Single, Instanced };

inline ModelType modelTypeFromString(const std::string& typeStr) {
    if (typeStr == "single") return ModelType::Single;
    if (typeStr == "instanced") return ModelType::Instanced;

    std::cout << "Cannot find model type " << typeStr << std::endl;
    return ModelType::Single;
}