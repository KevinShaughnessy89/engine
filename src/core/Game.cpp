#include "Game.hpp"

#include <tracy/Tracy.hpp>

#include "CommandBuffer.hpp"
#include "Config.hpp"
#include "Core.hpp"
#include "FrameCounter.hpp"
#include "audio/AudioController.hpp"
#include "character/CharacterController.hpp"
#include "collision/CollisionManager.hpp"
#include "components/collision/ShapeData.hpp"
#include "components/environment/SkyState.hpp"
#include "components/physics/JustCreated.hpp"
#include "components/physics/KinematicBody.hpp"
#include "components/rendering/CameraState.hpp"
#include "components/rendering/InstancedRenderable.hpp"
#include "components/rendering/Renderable.hpp"
#include "components/rendering/SunState.hpp"
#include "components/rendering/ViewState.hpp"
#include "core/InputHandler.hpp"
#include "core/KeyHandling.hpp"
#include "core/ModuleManager.hpp"
#include "core/MouseHandling.hpp"
#include "core/PrecompiledHeader.hpp"
#include "core/SystemUtilities.hpp"
#include "environment/ChunkManager.hpp"
#include "environment/EnvironmentManager.hpp"
#include "gui/MainMenu.hpp"
#include "imgui.h"
#include "physics/PhysicsManager.hpp"
#include "physics/ProjectileManager.hpp"
#include "rendering/CameraController.hpp"
#include "rendering/DebugState.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/DisplayState.hpp"
#include "rendering/KinematicController.hpp"
#include "rendering/LightManager.hpp"
#include "rendering/ModelLoader.hpp"
#include "rendering/ResourceManager.hpp"
#include "rendering/ShapeUpdater.hpp"
#include "rendering/TextureManager.hpp"
#include "rendering/sky/SunHelper.hpp"

GameState Game::state = GameState::MAIN_MENU;
bool Game::showDebugPanel = false;
FixedRateClock* Game::clock120 = nullptr;
FixedRateClock* Game::clock240 = nullptr;
bool Game::paused = false;
bool Game::stepRequested = false;

void Game::setPaused(bool shouldPause) {
    if (paused == shouldPause) return;
    paused = shouldPause;
    // Same stop()/start() pattern ChunkManager::regenerateTerrain() uses to keep the
    // physics/collision worker threads from ticking against state the debug panel is inspecting.
    if (paused) {
        if (clock120) clock120->stop();
        if (clock240) clock240->stop();
    } else {
        if (clock120) clock120->start();
        if (clock240) clock240->start();
    }
}

Game::Game() {
    systemCore = new Core();
}

void Game::onKeyEvent(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS && (mods & GLFW_MOD_ALT)) {
        toggleFullscreen(window);
    }
}

void Game::onContinuousInput(double deltaTime, const KeyHandling& keyHandler) {
}

void GLAPIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                              GLsizei length, const GLchar* message, const void* userParam) {
    std::cerr << "-----------------------------\n";

    std::cerr << "GL DEBUG MESSAGE\n";
    std::cerr << "ID: " << id << "\n";
    std::cerr << "Message: " << message << "\n";

    std::cerr << "Source: ";
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            std::cerr << "API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            std::cerr << "Window System";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            std::cerr << "Shader Compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            std::cerr << "Third Party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            std::cerr << "Application";
            break;
        default:
            std::cerr << "Other";
            break;
    }
    std::cerr << "\n";

    std::cerr << "Type: ";
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            std::cerr << "Error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            std::cerr << "Deprecated";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            std::cerr << "Undefined Behavior";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            std::cerr << "Portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            std::cerr << "Performance";
            break;
        case GL_DEBUG_TYPE_MARKER:
            std::cerr << "Marker";
            break;
        default:
            std::cerr << "Other";
            break;
    }
    std::cerr << "\n";

    std::cerr << "Severity: ";
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            std::cerr << "HIGH";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            std::cerr << "MEDIUM";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            std::cerr << "LOW";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            std::cerr << "NOTIFICATION";
            break;
        default:
            std::cerr << "UNKNOWN";
            break;
    }
    std::cerr << "\n";

    std::cerr << "-----------------------------\n";
}

void Game::focus_callback(GLFWwindow* window, int focused) {
    if (focused) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glfwSetCursorPos(window, width * 0.5f, height * 0.5f);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

bool Game::init() {
    Game::state = GameState::MAIN_MENU;

    std::cout << "Initializing GLFW...\n";
    if (!glfwInit()) {
        std::cout << "Error initializing GLFW...\n";
    }

    std::cout << "Setting GLFW window hints...\n";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);  // request a 24-bit depth buffer
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    std::cout << "Creating GLFW window...\n";
    this->window =
        glfwCreateWindow(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "Engine", NULL, NULL);
    DisplayState::window = window;
    if (window == NULL) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    std::cout << "Window created successfully.\n";

    std::cout << "Making context current...\n";
    glfwMakeContextCurrent(window);
    // Without this, frame pacing is left to OS/driver defaults, which differ between windowed
    // (implicitly paced by DWM composition) and exclusive fullscreen (DWM bypassed, uncapped) -
    // that inconsistency in deltaTime is what was causing fullscreen-only physics jitter.
    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    std::cout << "Initializing GLEW...\n";
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
        return false;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugCallback, nullptr);

    /* if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    */

    std::cout << "GLEW initialized.\n";

    std::cout << "Setting stb_image flip...\n";
    stbi_set_flip_vertically_on_load(true);

    std::cout << "Setting OpenGL viewport and state...\n";
    glViewport(0.f, 0.f, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glEnable(GL_PRIMITIVE_RESTART);
    glClearDepth(1.0);
    glPrimitiveRestartIndex(0xFFFF);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    // force=true: this is the first time any GLStateFlags-tracked capability is ever touched, so
    // there's no real prior GL state to diff against -- GLState::current's field defaults describe
    // the state entities typically want (e.g. cullFace=true), not the real GL context defaults
    // (cull face starts disabled), so a diff-based applyState() here would wrongly skip calls for
    // any field that already matches those defaults.
    GLStateFlags initialState = GLState::current;
    GLState::applyState(initialState, /*force=*/true);

    std::cout << "OpenGL state initialized.\n";

    std::cout << glGetString(GL_VERSION) << std::endl;

    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0);
    // Initialize core system
    std::cout << glCheckError() << std::endl;
    // MODULES
    Modules.registerModule(&Textures, "TextureManager");
    // AUDIO
    Audio.init();
    // DISPLAY
    Display.loadShaders();
    Display.setAspectValues(framebufferWidth, framebufferHeight);
    // Register clock callbacks
    clock120 = new FixedRateClock(120);
    clock240 = new FixedRateClock(120);

    clock240->registerCallback([](float deltaTime) {
        static ShapeUpdater shapeUpdater;
        float dt = static_cast<float>(deltaTime) / 1e9;
        Physics.runPhysics(dt);
        shapeUpdater.update();
        Collision.processCollisions(dt);
    });

    // EVENT HANDLING
    Input.addKeyObserver(&systemCore->movementController);
    Input.addKeyObserver(&systemCore->systemKeyBinder);
    Input.addKeyObserver(&Projectiles);
    Input.addMouseObserver(&Projectiles);
    Input.addMouseObserver(&systemCore->movementController);

    // ASSET MANAGEMENT

    Resource.buildProjectiles();
    Resource.setCameraValues();

    Modules.startInitialization();

    // Collision.initOctTree();
    //  m_ResourceManager.buildEnvironmentRegistry(m_EnvironmentManager);
    CameraController::init();
    Physics.init();
    // WINDOW AND CALLBACKS
    mainMenu.init(window);

    glfwSetWindowUserPointer(window, &Input);
    glfwSetKeyCallback(window, Input.keyboard_callback_);
    glfwSetCursorPosCallback(window, Input.mouseCursor_callback_);
    glfwSetMouseButtonCallback(window, Input.mouseButton_callback_);
    glfwSetWindowFocusCallback(window, focus_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    Mouse.printObservers();

    std::shared_future<void> textureFuture = Modules.getReadyFuture("TextureManager");
    textureFuture.wait();

    // Main context GL calls
    Textures.loadTextures();  // Environment.init() depends on textures being done
    Projectiles.init();

    Display.setRendererWindowHeight(framebufferHeight);
    Display.setRendererWindowWidth(framebufferWidth);

    ModelRegistry.loadScene(Config::ProjectRootDir + "/" + Config::ModelsPath);

    prewarmComponentStorage();

    // Environment.init() also depends on ModelRegistry.loadScene() having run, since it
    // attaches SunState/SkyState to the "sun"/"skybox" entities looked up there.
    Environment.init();
    systemCore->characterController.init();
    Display.init();

    return true;
}

// Pre-warm all component pools that are accessed by worker threads.
// EnTT's assure() writes to an internal dense_map on first access of any
// component type, even for get() calls. All pools must be initialized on
// the main thread before any worker threads start accessing the registry.
void Game::prewarmComponentStorage() {
    Registry.storage<CameraState>();
    Registry.storage<KinematicBody>();
    Registry.storage<ShapeData>();
    Registry.storage<Renderable>();
    Registry.storage<Position>();
    Registry.storage<Velocity>();
    Registry.storage<Acceleration>();
    Registry.storage<Damping>();
    Registry.storage<Force>();
    Registry.storage<InverseMass>();
    Registry.storage<Inertia>();
    Registry.storage<Orientation>();
    Registry.storage<Sleep>();
    Registry.storage<Torque>();
    Registry.storage<SunState>();
    Registry.storage<SkyState>();
    Registry.storage<InstancedRenderable>();
    Registry.storage<JustCreated>();
    Registry.storage<ChunkRenderable>();
    Registry.storage<ViewState>();
}

void Game::runMainMenu() {
    switch (state) {
        case GameState::MAIN_MENU:
            state = updateMenu();
            break;

        case GameState::LOADING:
            state = updateLoading();
        case GameState::RUNNING:
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            break;
    }
}

GameState Game::updateLoading() {
    Chunks.startIfNeeded();
    Chunks.update();
    return mainMenu.renderLoadingScreen(Chunks.getLoadingProgress());
}

GameState Game::updateMenu() {
    return mainMenu.renderMainMenu();
}

void Game::updateWorldState(double deltaTime, double currentTime) {
    ZoneScoped;
    ImGuiIO& io = ImGui::GetIO();

    {
        ZoneScopedN("updateWorldState: input + camera");
        Input.processContinuousKeyboardInput(deltaTime);
        if (!io.WantCaptureMouse) {
            CameraController::update(deltaTime);
            systemCore->kinematicController.update(deltaTime);
            systemCore->characterController.update(deltaTime);
        }
    }
    {
        ZoneScopedN("updateWorldState: Chunks.update");
        Chunks.update();
    }
    {
        ZoneScopedN("updateWorldState: Environment.update");
        Environment.update(deltaTime);
    }
    {
        ZoneScopedN("updateWorldState: Commands.processEntityCreation");
        Commands.processEntityCreation();
    }

    if (!DebugState::sunManualOverride) {
        ZoneScopedN("updateWorldState: SunHelper::updateSunState");
        SunHelper::updateSunState(Registry.get<SunState>(ModelRegistry.getEntity("sun")));
    }

    Global::frameCounter++;
}

double renderTime = glfwGetTime();
double lastRenderTime = glfwGetTime();

void Game::updateMainThread(double deltaTime, double currentTime) {
    switch (state) {
        case GameState::ENTRY:
            mainMenu.runEntrySplash();
            state = GameState::INIT;
            break;
        case GameState::INIT:
            init();
            state = GameState::MAIN_MENU;
            break;
        case GameState::MAIN_MENU:
        case GameState::LOADING:
            runMainMenu();
            break;
        case GameState::RUNNING: {
            ZoneScopedN("updateMainThread: RUNNING");
            if (!clocksStarted) {
                clock120->start();
                clock240->start();
                clocksStarted = true;
            }
            if (!paused || stepRequested) {
                updateWorldState(deltaTime, currentTime);
                stepRequested = false;
            }
            {
                ZoneScopedN("updateMainThread: Display.render");
                Display.render(deltaTime);
            }
            {
                ZoneScopedN("updateMainThread: Registry.clear<JustCreated>");
                Registry.clear<JustCreated>();
            }
            double now = glfwGetTime();
            renderTime = now - lastRenderTime;
            lastRenderTime = now;
            break;
        }
        case GameState::QUIT:
            glfwTerminate();
            break;
    }
}

double lastSwapTime = glfwGetTime();
double swapTime = glfwGetTime();

double lastUpdateTime = glfwGetTime();
double updateTime = glfwGetTime();

void Game::run() {
    float lastFrameTime = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        ZoneScopedN("Game::run: frame");
        {
            ZoneScopedN("Game::run: ImGui new frame");
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        if (deltaTime > 0.0) {
            DebugState::currentFPS = static_cast<float>(1.0 / deltaTime);
        }
        updateMainThread(deltaTime, currentTime);
        {
            ZoneScopedN("Game::run: debug panel");
            if (showDebugPanel) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                debugPanel.renderDebugPanel();
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            debugPanel.renderOverlay();
        }
        double now = glfwGetTime();
        double dtUpdate = now - lastUpdateTime;
        lastUpdateTime = now;
        {
            ZoneScopedN("Game::run: glfwPollEvents");
            glfwPollEvents();
        }
        {
            ZoneScopedN("Game::run: ImGui render");
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        {
            ZoneScopedN("Game::run: glfwSwapBuffers");
            glfwSwapBuffers(window);
        }
        now = glfwGetTime();
        double swapTime = now - lastSwapTime;
        lastSwapTime = now;
        FrameMark;
    }

    glfwTerminate();
}

GLenum Game::glCheckError_(const char* file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                error = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                error = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
