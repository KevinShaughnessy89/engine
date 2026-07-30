#include "ResourceManager.hpp"

#include "collision/shapes/ColliderFactory.hpp"
#include "components/rendering/Renderable.hpp"
#include "core/Config.hpp"
#include "core/Core.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "physics/ProjectileManager.hpp"
#include "rendering/CameraConstants.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/LightManager.hpp"
#include "rendering/LightSource.hpp"
#include "rendering/Model.hpp"
#include "rendering/ModelLoader.hpp"
#include "rendering/TextureManager.hpp"

ResourceManager::ResourceManager() {
    std::cout << "[ResourceManager] Loading Object JSON from " << Config::ObjectPath << "\n";
    loadJSONFromFile(Config::ProjectRootDir + "/" + Config::ObjectPath, object);
    std::cout << "[ResourceManager] Loaded Object JSON\n";

    std::cout << "[ResourceManager] Loading Projectile JSON from " << Config::ProjectilePath
              << "\n";
    loadJSONFromFile(Config::ProjectRootDir + "/" + Config::ProjectilePath, projectile);
    std::cout << "[ResourceManager] Loaded Projectile JSON\n";

    std::cout << "[ResourceManager] Loading Light JSON from " << Config::LightPath << "\n";
    loadJSONFromFile(Config::ProjectRootDir + "/" + Config::LightPath, light);
    std::cout << "[ResourceManager] Loaded Light JSON\n";

    std::cout << "[ResourceManager] Loading Camera JSON from " << Config::CameraPath << "\n";
    loadJSONFromFile(Config::ProjectRootDir + "/" + Config::CameraPath, camera);
    std::cout << "[ResourceManager] Loaded Camera JSON\n";

    std::cout << "[Enemy] Loading Enemy JSON from " << Config::EnemyPath << "\n";
    loadJSONFromFile(Config::ProjectRootDir + "/" + Config::EnemyPath, enemy);
    std::cout << "[ResourceManager] Loaded Enemy JSON\n";

    std::cout << "[ResourceManager] Parsing Object Registry\n";
    this->ObjectRegistry = parseObjectsFromJSON(object);
    std::cout << "[ResourceManager] Parsed Object Registry\n";

    std::cout << "[ResourceManager] Parsing Projectile Registry\n";
    this->ProjectileRegistry = parseProjectilesFromJSON(projectile);
    std::cout << "[ResourceManager] Parsed Projectile Registry\n";

    std::cout << "[ResourceManager] Parsing Light Source Registry\n";
    this->LightSourceRegistry = parseLightSourcesFromJSON(light);
    std::cout << "[ResourceManager] Parsed Light Source Registry\n";

    // std::cout << "[Enemy] Parsing Enemy Registry\n";
    // this->EnemyRegistry = parseEnemiesFromJSON(enemy);
    // std::cout << "[ResourceManager] Parsed Enemy Registry\n";

    // std::cout << "[ResourceManager] Parsing Environment Registry\n";
    // this->EnvironmentRegistry = parseEnvironmentFromJSON(environment);
    // std::cout << "[ResourceManager] Parsed Environment Registry\n";

    std::cout << "[ResourceManager] Finished constructor\n";
}

bool ResourceManager::loadJSONFromFile(const std::string& filename, rapidjson::Document& doc) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }

    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content.append(line + "\n");
    }
    file.close();

    doc.Parse(content.c_str(), content.length());
    return doc.IsObject() || doc.IsArray();
}

std::unordered_set<ObjectAsset> ResourceManager::parseObjectsFromJSON(
    const rapidjson::Document& doc) {
    std::unordered_set<ObjectAsset> objects;
    const rapidjson::Value& objectsArray = doc;

    for (rapidjson::SizeType i = 0; i < objectsArray.Size(); i++) {
        const rapidjson::Value& object = objectsArray[i];
        if (!object.IsObject()) {
            std::cerr << "Error: Expected object in JSON array!" << std::endl;
            continue;
        }
        ObjectAsset gameAsset;
        gameAsset.UID = object["UID"].GetString();
        gameAsset.filepath = object["filepath"].GetString();
        gameAsset.shaderType = shaderTypeFromString(object["shaderType"].GetString());
        gameAsset.inverseMass = object["inverseMass"].GetFloat();
        gameAsset.staticFlag = object["staticFlag"].GetInt();
        const rapidjson::Value& position = object["position"];
        gameAsset.position.x = position[0].GetFloat();
        gameAsset.position.y = position[1].GetFloat();
        gameAsset.position.z = position[2].GetFloat();
        gameAsset.boundingVolume =
            ColliderFactory::fromString(object["boundingVolume"].GetString());
        if (object.HasMember("textures") && object["textures"].IsArray()) {
            const rapidjson::Value& texturesArray = object["textures"];
            for (rapidjson::SizeType j = 0; j < texturesArray.Size(); j++) {
                gameAsset.textures.push_back(texturesArray[j].GetString());
            }
        }
        objects.insert(gameAsset);
    }
    return objects;
}

std::unordered_set<ProjectileAsset> ResourceManager::parseProjectilesFromJSON(
    const rapidjson::Document& doc) {
    std::unordered_set<ProjectileAsset> projectiles;
    const rapidjson::Value& projectileArray = doc;

    for (rapidjson::SizeType i = 0; i < projectileArray.Size(); i++) {
        const rapidjson::Value& object = projectileArray[i];
        if (!object.IsObject()) {
            std::cerr << "Error: Expected object in JSON array!" << std::endl;
            continue;
        }

        ProjectileAsset gameAsset;
        gameAsset.UID = object["UID"].GetString();
        gameAsset.inverseMass = object["inverseMass"].GetFloat();
        gameAsset.filepath = object["filepath"].GetString();
        const rapidjson::Value& velocity = object["velocity"];
        gameAsset.velocity = object["velocity"].GetFloat();
        gameAsset.acceleration = object["acceleration"].GetFloat();
        gameAsset.force = object["force"].GetFloat();
        gameAsset.staticFlag = object["staticFlag"].GetInt();
        const rapidjson::Value& position = object["position"];
        gameAsset.position.x = position[0].GetFloat();
        gameAsset.position.y = position[1].GetFloat();
        gameAsset.position.z = position[2].GetFloat();
        gameAsset.boundingVolume =
            ColliderFactory::fromString(object["boundingVolume"].GetString());
        projectiles.insert(gameAsset);
    }
    return projectiles;
}

std::unordered_set<EnvironmentAsset> ResourceManager::parseEnvironmentFromJSON(
    const rapidjson::Document& doc) {
    std::unordered_set<EnvironmentAsset> environment;
    const rapidjson::Value& environmentArray = doc;
    for (rapidjson::SizeType i = 0; i < environmentArray.Size(); i++) {
        const rapidjson::Value& object = environmentArray[i];
        if (!object.IsObject()) {
            std::cerr << "Error: Expected object in JSON array!" << std::endl;
            continue;
        }

        EnvironmentAsset gameAsset;
        gameAsset.UID = object["UID"].GetString();
        gameAsset.boundingVolume = object["boundingVolume"].GetString();
        gameAsset.texturePath = object["texturePath"].GetString();
        gameAsset.type = object["type"].GetString();
        gameAsset.size = object["size"].GetFloat();
        gameAsset.xOffset = object["xOffset"].GetFloat();
        gameAsset.yOffset = object["yOffset"].GetFloat();
        gameAsset.zOffset = object["zOffset"].GetFloat();
        environment.insert(gameAsset);
    }
    return environment;
}

// std::unordered_set<EnemyAsset> ResourceManager::parseEnemiesFromJSON(
//     const rapidjson::Document& doc) {
//     std::unordered_set<EnemyAsset> enemies;
//     const rapidjson::Value& enemyArray = doc;
//     for (rapidjson::SizeType i = 0; i < enemyArray.Size(); i++) {
//         const rapidjson::Value& object = enemyArray[i];
//         if (!object.IsObject()) {
//             std::cerr << "Error: Expected object in JSON array!" << std::endl;
//             continue;
//         }

//         EnemyAsset gameAsset;
//         gameAsset.UID = object["UID"].GetString();
//         gameAsset.boundingVolume = getShapeTypeFromJSON(object);
//         gameAsset.filepath = object["filepath"].GetString();
//         gameAsset.health = object["health"].GetInt();
//         const rapidjson::Value& position = object["position"];
//         gameAsset.position.x = position[0].GetFloat();
//         gameAsset.position.y = position[1].GetFloat();
//         gameAsset.position.z = position[2].GetFloat();
//         gameAsset.inverseMass = object["inverseMass"].GetFloat();
//         enemies.insert(gameAsset);
//     }
//     return enemies;
// }

std::unordered_set<LightAsset> ResourceManager::parseLightSourcesFromJSON(
    const rapidjson::Document& doc) {
    std::unordered_set<LightAsset> lights;
    const rapidjson::Value& lightArray = doc;

    for (rapidjson::SizeType i = 0; i < lightArray.Size(); i++) {
        const rapidjson::Value& object = lightArray[i];
        if (!object.IsObject()) {
            std::cerr << "Error: Expected object in JSON array!" << std::endl;
            continue;
        }

        LightAsset gameAsset;
        gameAsset.UID = object["UID"].GetString();
        gameAsset.filepath = object["filepath"].GetString();
        gameAsset.inverseMass = object["inverseMass"].GetFloat();
        gameAsset.initialAcceleration = object["initialAcceleration"].GetFloat();
        gameAsset.staticFlag = object["staticFlag"].GetInt();
        const rapidjson::Value& position = object["position"];
        gameAsset.position.x = position[0].GetFloat();
        gameAsset.position.y = position[1].GetFloat();
        gameAsset.position.z = position[2].GetFloat();
        gameAsset.type = object["type"].GetInt();
        const rapidjson::Value& direction = object["direction"];
        gameAsset.direction.x = direction[0].GetFloat();
        gameAsset.direction.y = direction[1].GetFloat();
        gameAsset.direction.z = direction[2].GetFloat();
        const rapidjson::Value& ambient = object["ambient"];
        gameAsset.ambient.x = ambient[0].GetFloat();
        gameAsset.ambient.y = ambient[1].GetFloat();
        gameAsset.ambient.z = ambient[2].GetFloat();
        const rapidjson::Value& diffuse = object["diffuse"];
        gameAsset.diffuse.x = diffuse[0].GetFloat();
        gameAsset.diffuse.y = diffuse[1].GetFloat();
        gameAsset.diffuse.z = diffuse[2].GetFloat();
        const rapidjson::Value& specular = object["specular"];
        gameAsset.specular.x = specular[0].GetFloat();
        gameAsset.specular.y = specular[1].GetFloat();
        gameAsset.specular.z = specular[2].GetFloat();
        gameAsset.linear = object["linear"].GetFloat();
        gameAsset.quadratic = object["quadratic"].GetFloat();
        gameAsset.constant = object["constant"].GetFloat();
        gameAsset.cutOff = object["cutoff"].GetFloat();
        gameAsset.outerCutOff = object["outerCutOff"].GetFloat();

        lights.insert(gameAsset);
    }
    return lights;
}

// PhysicsManager should be responsible for lifetime management of these pointers
void ResourceManager::buildObjects() {
    // for (const auto& object : ObjectRegistry) {
    //     if (object.filepath.compare("null") != 0) {
    //         ModelRegistry.create(object.UID, object.filepath, object.shaderType,
    //                              RenderLayer::Objects, ModelType::Single);

    //         if (ModelRegistry.entities[object.UID].empty()) {
    //             std::cout << "Warning: failed to load model for object \"" << object.UID
    //                       << "\"; skipping." << std::endl;
    //             continue;
    //         }
    //         Model* model =
    //             Registry.get<Renderable>(ModelRegistry.entities[object.UID].front()).model;

    //         for (const std::string& textureName : object.textures) {
    //             Texture* texture = Textures.getTextureAs(textureName, textureName);
    //             for (auto& mesh : model->meshes) {
    //                 mesh->textures.push_back(texture);
    //             }
    //         }

    //         GLuint shaderId = Display.getShaderIdFromShaderType(object.shaderType);
    //         addModel(shaderId, model);
    //     }
    // }
}
void ResourceManager::buildProjectiles() {
    for (const auto& projectile : ProjectileRegistry) {
        Projectiles.addProjectileAsset(projectile);
    }
}

void ResourceManager::buildLightSources() {
    //   Display.getRenderer().addGlobalUniforms(
    //     "lights",
    //     std::make_pair(std::string("view"), Camera.getViewMatrix()),
    //     std::make_pair(std::string("projection"), Display.getPerspectiveMatrix())
    //   );

    // for (const auto& light : LightSourceRegistry) {

    //   LightSource* lightInst = new LightSource(light.UID, model, light.type, light.position,
    //   light.direction, light.ambient, light.diffuse, light.specular, light.constant,
    //   light.linear,
    //                     light.quadratic, light.cutOff, light.outerCutOff);
    //   Light.addLightSource(lightInst);
    // }
}

void ResourceManager::setCameraValues() {
    const rapidjson::Value& valueArray = this->camera;

    for (rapidjson::SizeType i = 0; i < valueArray.Size(); i++) {
        const rapidjson::Value& object = valueArray[i];
        if (!object.IsObject()) {
            std::cerr << "Error: Expected object in JSON array!" << std::endl;
            continue;
        }
        CameraConstants::maxYawRate = object["maxYaw"].GetFloat();
        CameraConstants::maxPitchRate = object["maxPitch"].GetFloat();
    }
}

void ResourceManager::getAssetRegistryInfo() {
    std::cout << "Asset registry info: " << std::endl;
    for (auto i = ObjectRegistry.begin(); i != ObjectRegistry.end(); i++) {
        std::cout << i->UID << " " << i->filepath << std::endl;
    }
    for (auto i = ProjectileRegistry.begin(); i != ProjectileRegistry.end(); i++) {
        std::cout << i->UID << std::endl;
    }
    for (auto i = LightSourceRegistry.begin(); i != LightSourceRegistry.end(); i++) {
        std::cout << i->UID << " " << i->filepath << std::endl;
    }
}

void ResourceManager::addModel(GLuint shaderKey, Model* model) {
    modelRegistry[shaderKey] = model;
}