#pragma once
#include <future>

#include "core/PrecompiledHeader.hpp"
#include "physics/ProjectileManagerStructs.hpp"
#include "rendering/ShaderType.hpp"

class Model;
class PhysicsObject;
class LightSource;
class Projectile;
enum class CollisionType : int;

struct ObjectAsset {
    std::string UID;
    std::string filepath;
    ShaderType shaderType;
    float inverseMass;
    float initialAcceleration;
    int staticFlag;
    glm::vec4 position;
    CollisionType boundingVolume;
    std::vector<std::string> textures;

    ObjectAsset() { UID, filepath = ""; }

    bool operator==(const ObjectAsset& T) const {
        return (this->UID == T.UID && this->filepath == T.filepath);
    }

    size_t operator()(const ObjectAsset& T) const { return std::hash<std::string>()(T.UID); }
};

struct EnvironmentAsset {
    std::string UID;
    std::string boundingVolume;
    std::string texturePath;
    std::string type;
    float size;
    float xOffset;
    float yOffset;
    float zOffset;

    EnvironmentAsset() { UID, boundingVolume = ""; }
    bool operator==(const EnvironmentAsset& T) const {
        return (this->UID == T.UID && this->boundingVolume == T.boundingVolume);
    }

    size_t operator()(const EnvironmentAsset& T) const { return std::hash<std::string>()(T.UID); }
};

struct LightAsset {
    std::string UID;
    std::string filepath;
    float inverseMass;
    float initialAcceleration;
    int staticFlag;
    glm::vec4 position;
    int type;
    glm::vec4 direction;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    float constant;
    float linear;
    float quadratic;
    float shininness;
    float cutOff;
    float outerCutOff;

    LightAsset() { UID, filepath = ""; }

    bool operator==(const LightAsset& T) const {
        return (this->UID == T.UID && this->filepath == T.filepath);
    }

    size_t operator()(const LightAsset& T) const { return std::hash<std::string>()(T.UID); }
};

// namespace std {
// template <>
// struct hash<EnemyAsset> {
//     std::size_t operator()(const EnemyAsset& asset) const noexcept { return asset(asset); }
// };
// }  // namespace std

namespace std {
template <>
struct hash<ObjectAsset> {
    std::size_t operator()(const ObjectAsset& asset) const noexcept { return asset(asset); }
};
}  // namespace std

namespace std {
template <>
struct hash<ProjectileAsset> {
    std::size_t operator()(const ProjectileAsset& asset) const noexcept { return asset(asset); }
};
}  // namespace std

namespace std {
template <>
struct hash<LightAsset> {
    std::size_t operator()(const LightAsset& asset) const noexcept { return asset(asset); }
};
}  // namespace std

namespace std {
template <>
struct hash<EnvironmentAsset> {
    std::size_t operator()(const EnvironmentAsset& asset) const noexcept { return asset(asset); }
};
}  // namespace std

class ResourceManager {
   public:
    ResourceManager();
    void buildAssetRegistry();
    void buildObjects();
    void buildProjectiles();
    void buildLightSources();
    void setCameraValues();
    void getAssetRegistryInfo();
    void addTextureAsync(std::future<void>&& textureProcess);
    void processLoadingTextures();

    bool loadJSONFromFile(const std::string& filename, rapidjson::Document& doc);
    std::unordered_set<ObjectAsset> parseObjectsFromJSON(const rapidjson::Document& doc);
    std::unordered_set<ProjectileAsset> parseProjectilesFromJSON(const rapidjson::Document& doc);
    std::unordered_set<LightAsset> parseLightSourcesFromJSON(const rapidjson::Document& doc);
    std::unordered_set<EnvironmentAsset> parseEnvironmentFromJSON(const rapidjson::Document& doc);
    // std::unordered_set<EnemyAsset> parseEnemiesFromJSON(const rapidjson::Document& doc);

   private:
    rapidjson::Document object;
    rapidjson::Document projectile;
    rapidjson::Document light;
    rapidjson::Document camera;
    rapidjson::Document environment;
    rapidjson::Document enemy;

    std::unordered_set<ObjectAsset> ObjectRegistry;
    std::unordered_set<ProjectileAsset> ProjectileRegistry;
    std::unordered_set<LightAsset> LightSourceRegistry;
    // std::unordered_set<EnemyAsset> EnemyRegistry;

    std::unordered_map<GLuint, Model*> modelRegistry;
    void addModel(GLuint shaderKey, Model* model);
};
