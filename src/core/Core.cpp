#include "Core.hpp"

#include "ai/EnemyManager.hpp"
#include "audio/AudioController.hpp"
#include "collision/CollisionManager.hpp"
#include "collision/ContactResolver.hpp"
#include "core/CommandBuffer.hpp"
#include "core/InputHandler.hpp"
#include "core/KeyHandling.hpp"
#include "core/ModuleManager.hpp"
#include "core/MouseHandling.hpp"
#include "environment/ChunkManager.hpp"
#include "environment/EnvironmentManager.hpp"
#include "environment/TriangleManager.hpp"
#include "physics/PhysicsManager.hpp"
#include "physics/ProjectileManager.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/KinematicController.hpp"
#include "rendering/LightManager.hpp"
#include "rendering/ModelLoader.hpp"
#include "rendering/ResourceManager.hpp"
#include "rendering/TextureManager.hpp"

CollisionManager& Collision = Core::Collision();
EnvironmentManager& Environment = Core::Environment();
ChunkManager& Chunks = Core::Chunk();
PhysicsManager& Physics = Core::Physics();
ProjectileManager& Projectiles = Core::Projectile();
LightManager& Light = Core::Light();
ResourceManager& Resource = Core::Resource();
ContactResolver& Contact = Core::Contact();
KeyHandling& Keyboard = Core::Keyboard();
MouseHandling& Mouse = Core::Mouse();
InputHandler& Input = Core::Input();
DisplayManager& Display = Core::Display();
AudioController& Audio = Core::Audio();
TextureManager& Textures = Core::Textures();
ModuleManager& Modules = Core::Modules();
EnemyManager& Enemies = Core::Enemies();
CommandBuffer& Commands = Core::Commands();
TriangleManager& Triangles = Core::Triangles();
ModelLoader& ModelRegistry = Core::Models();
entt::registry& Registry = Core::Registry();

TextureManager& Core::Textures() {
    static TextureManager instance;
    std::cout << "[Core] TextureManager constructed\n";
    return instance;
}

AudioController& Core::Audio() {
    static AudioController instance;
    std::cout << "[Core] AudioController constructed\n";
    return instance;
}

CollisionManager& Core::Collision() {
    static CollisionManager instance;
    std::cout << "[Core] CollisionManager constructed\n";
    return instance;
}

EnvironmentManager& Core::Environment() {
    static EnvironmentManager instance;
    std::cout << "[Core] EnvironmentManager constructed\n";
    return instance;
}

ChunkManager& Core::Chunk() {
    static ChunkManager instance;
    std::cout << "[Core] ChunkManager constructed\n";
    return instance;
}

PhysicsManager& Core::Physics() {
    static PhysicsManager instance;
    std::cout << "[Core] PhysicsManager constructed\n";
    return instance;
}

ProjectileManager& Core::Projectile() {
    static ProjectileManager instance;
    std::cout << "[Core] ProjectileManager constructed\n";
    return instance;
}

LightManager& Core::Light() {
    static LightManager instance;
    std::cout << "[Core] LightManager constructed\n";
    return instance;
}

ResourceManager& Core::Resource() {
    static ResourceManager instance;
    std::cout << "[Core] ResourceManager constructed\n";
    return instance;
}

ContactResolver& Core::Contact() {
    static ContactResolver instance;
    std::cout << "[Core] ContactResolver constructed\n";
    return instance;
}

InputHandler& Core::Input() {
    static InputHandler instance;
    std::cout << "[Core] InputHandler constructed\n";
    return instance;
}

MouseHandling& Core::Mouse() {
    static MouseHandling instance;
    std::cout << "[Core] MouseHandling constructed\n";
    return instance;
}

KeyHandling& Core::Keyboard() {
    static KeyHandling instance;
    std::cout << "[Core] KeyHandling constructed\n";
    return instance;
}

DisplayManager& Core::Display() {
    static DisplayManager instance;
    std::cout << "[Core] DisplayManager constructed\n";
    return instance;
}

ModuleManager& Core::Modules() {
    static ModuleManager instance;
    std::cout << "[Core] ModuleManager constructed\n";
    return instance;
}

EnemyManager& Core::Enemies() {
    static EnemyManager instance;
    std::cout << "[Core] EnemyManager constructed\n";
    return instance;
}

CommandBuffer& Core::Commands() {
    static CommandBuffer instance;
    std::cout << "[Core] CommandBuffer constructed\n";
    return instance;
}

TriangleManager& Core::Triangles() {
    static TriangleManager instance;
    std::cout << "[Core] TriangleManager constructed\n";
    return instance;
}

ModelLoader& Core::Models() {
    static ModelLoader instance;
    std::cout << "[Core] ModelLoader constructed\n";
    return instance;
}

entt::registry& Core::Registry() {
    static entt::registry instance;
    return instance;
}
