#pragma once
#include "collision/CollisionType.hpp"
#include "core/PrecompiledHeader.hpp"


struct ProjectileAsset {
    std::string UID;
    float inverseMass;
    float force;
    int staticFlag;
    glm::vec4 position;
    float velocity;
    float acceleration;
    CollisionType boundingVolume;
    std::string filepath;

    ProjectileAsset() { UID = ""; }

    bool operator==(const ProjectileAsset& T) const { return (this->UID == T.UID); }

    size_t operator()(const ProjectileAsset& T) const { return std::hash<std::string>()(T.UID); }
};
