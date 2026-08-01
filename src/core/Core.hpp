#pragma once
#include "character/CharacterController.hpp"
#include "core/SystemKeyBinder.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "rendering/KinematicController.hpp"
#include "rendering/MovementController.hpp"


class AudioManager;
class CollisionManager;
class EnvironmentManager;
class ChunkManager;
class PhysicsManager;
class ProjectileManager;
class LightManager;
class ResourceManager;
class ContactResolver;
class InputHandler;
class MouseHandling;
class KeyHandling;
class DisplayManager;
class TextureManager;
class ModuleManager;
class EnemyManager;
class WorldObjects;
class CommandBuffer;
class TriangleManager;
class ModelLoader;

class Core {
   public:
    Core() {}
    Core& operator=(const Core&) = delete;
    Core(Core&&) = delete;
    Core& operator=(Core&&) = delete;

    KinematicController kinematicController;
    CharacterController characterController;
    MovementController movementController;
    SystemKeyBinder systemKeyBinder;

    static TextureManager& Textures();
    static AudioManager& Audio();
    static CollisionManager& Collision();
    static EnvironmentManager& Environment();
    static ChunkManager& Chunk();
    static PhysicsManager& Physics();
    static ProjectileManager& Projectile();
    static LightManager& Light();
    static ResourceManager& Resource();
    static ContactResolver& Contact();
    static InputHandler& Input();
    static MouseHandling& Mouse();
    static KeyHandling& Keyboard();
    static DisplayManager& Display();
    static ModuleManager& Modules();
    static EnemyManager& Enemies();
    static CommandBuffer& Commands();
    static TriangleManager& Triangles();
    static ModelLoader& Models();
    static entt::registry& Registry();
};

extern CollisionManager& Collision;
extern EnvironmentManager& Environment;
extern ChunkManager& Chunks;
extern PhysicsManager& Physics;
extern ProjectileManager& Projectiles;
extern LightManager& Light;
extern ResourceManager& Resource;
extern ContactResolver& Contact;
extern KeyHandling& Keyboard;
extern MouseHandling& Mouse;
extern InputHandler& Input;
extern DisplayManager& Display;
extern AudioManager& Audio;
extern TextureManager& Textures;
extern ModuleManager& Modules;
extern EnemyManager& Enemies;
extern CommandBuffer& Commands;
extern TriangleManager& Triangles;
extern ModelLoader& ModelRegistry;
extern entt::registry& Registry;