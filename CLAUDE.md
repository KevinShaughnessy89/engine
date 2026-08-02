# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A Windows-only C++23 real-time 3D engine/game built directly on OpenGL 4.2 core profile + GLFW, using EnTT for
its ECS. It is a single executable (`engine`/`engine.exe`), not a library.

## Build

Toolchain: MSVC (`cl.exe`) + Ninja + vcpkg, configured via CMakePresets.json (preset name `windows`). The build
is hardcoded in CMakeLists.txt to `CMAKE_BUILD_TYPE RelWithDebInfo` (optimized `/O2` with `/Zi` symbols),
warnings suppressed (`/w`). Do not reintroduce a target-level `/Od` — MSVC's last `/O` flag wins, so it would
silently disable optimization for every TU (this happened once; terrain generation was ~5-10x slower for it).

```
cmake --preset windows
cmake --build build
```

The binary lands at `build/engine.exe`. There's also a top-level `.vscode/launch.json` MSVC debug config that
launches `build/engine.exe` directly.

vcpkg dependencies are declared in `vcpkg.json` (glfw3, assimp, glew, glm, eigen3, mimalloc, flatbuffers, entt).
**Note:** despite `entt` being listed there, the engine actually includes the vendored copy at
`external/entt-main/src/entt/entt.hpp` everywhere (`#include "entt-main/src/entt/entt.hpp"`), not the vcpkg
package — keep using that vendored path for any EnTT-related code.

Other vendored deps live under `external/` (git-ignored except via CMakeLists wiring): `imgui` (statically
linked, built as its own CMake target), `fmod` (audio, linked as a prebuilt lib + DLL copied post-build),
`stbi` (image loading, compiled directly into `engine`).

There is no automated test suite. `src/collision/QuickHull/Tests/` (a vendored third-party QuickHull demo) is
explicitly excluded from the build in `CMakeLists.txt` and is not wired to anything.

A separate Rust crate lives at `src/ai/rust/ai_infer` (`cdylib`, for future model-inference calls from C++ via
the C ABI declared in `ai_infer.hpp`). It is not part of the CMake build — build it independently with `cargo
build` from that directory. `infer()` is currently a stub that always returns 0.

Formatting: `.clang-format` (Google base style, 4-space indent, 100 col, left-aligned pointers).
Comments: keep them to a single line — prefer one line over a multi-line block, even when it means
trimming down the explanation.
Naming: no Hungarian-style `k` prefix for constants (e.g. `kMaxBones`) — doesn't match this
codebase's established style. Use plain camelCase (e.g. `maxBones`).

## Tooling notes

Inline PowerShell commands (the PowerShell tool) are capped at ~965 bytes — longer commands fail with a
"malformed syntax" / "Command too long for parsing" error before they ever reach PowerShell. For anything
beyond a one-liner (e.g. `Add-Type` blocks, window automation, screenshot capture), write the script to a
`.ps1` file first (Write tool, no length limit there), then invoke it with a short `& "path\to\script.ps1"`
command.

## Architecture

**Entry point / main loop.** `src/core/main.cpp` constructs a `Game` (`src/core/Game.hpp/.cpp`) and calls
`init()` then `run()`. `Game::run()` is the GLFW loop; `Game::update()` is a state machine over `GameState`
(`ENTRY -> INIT -> MAIN_MENU/LOADING -> RUNNING -> QUIT`). Once `RUNNING`, per-frame work goes through
`updateWorld()` (continuous input, camera, chunk streaming, environment) followed by `Display.render()`.
Physics and collision do **not** run on the variable-rate render tick — they're registered as callbacks on a
`FixedRateClock` (120Hz/240Hz clocks created in `Game::init()`; see `src/core/FixedRateClock.hpp`).

**Service locator (`Core`).** `src/core/Core.hpp/.cpp` is the hub every subsystem is reached through. It
exposes `Core::Xxx()` static accessors, each backed by a function-local (Meyer's singleton) instance, plus
`Core::Registry()` for the single global `entt::registry`. `Core.cpp` additionally defines bare global
reference aliases (`Collision`, `Environment`, `Chunks`, `Physics`, `Projectiles`, `Light`, `Resource`,
`Contact`, `Keyboard`, `Mouse`, `Input`, `Display`, `Audio`, `Textures`, `Modules`, `Enemies`, `Registry`) — most
call sites use these bare names rather than `Core::Xxx()`. When adding a new manager subsystem, wire it into
both `Core.hpp`'s accessor list and `Core.cpp`'s alias list to match this pattern.

**ECS (EnTT).** Components are plain structs under `src/components/{physics,rendering,collision,environment}/`.
There are no dedicated "system" classes in the classic ECS sense — the manager classes reached via `Core`
(e.g. `PhysicsManager`, `CollisionManager`, `EnvironmentManager`) own the logic that iterates the registry.
Important gotcha: EnTT lazily allocates a component's storage on first access (`assure()`), even for `get()`
calls, and this must not race with worker threads. `Game::init()` explicitly pre-warms every component pool
used off the main thread (`Registry.storage<T>()` calls near the end of `init()`) — **any new component type
touched from worker threads must be added to that list**.

**Async module startup.** Subsystems with expensive startup work subclass `Module` (`src/core/Module.hpp`),
implement `doInitialization()`, and call `markReady()` when done. `ModuleManager` (`src/core/ModuleManager.hpp`)
registers modules by name and fires them concurrently via `std::async` in `startInitialization()`; other code
can block on a specific module via `Modules.getReadyFuture("Name").wait()` (see the `TextureManager` wait in
`Game::init()`) rather than assuming init order.

**Terrain / chunk streaming pipeline.** `ChunkManager` (`src/environment/ChunkManager.hpp`) owns a
`chunkGrid: unordered_map<glm::ivec2, unique_ptr<StaticTriangleSource>>` and delegates actual generation to
`ChunkGenerationPipeline`. The pipeline queues `ChunkTask`s (`creationTaskQueue` / `attributeGenerationQueue`),
dispatches mesh/height/biome generation asynchronously (`std::future<TerrainMeshData>` per chunk, produced by
`TerrainMeshGenerator` + `HeightMapGenerator` + `PerlinNoise`), and either builds terrain
fresh or loads it from disk via `TerrainSerializer`, which (de)serializes chunks using a flatbuffers schema
(`TerrainSerializationSchema.fbs`, with the generated header checked in alongside it). `ChunkBuilder` is the
current chunk-assembly abstraction (it replaced an earlier `Chunk` class to cut down on indirection — see git
history if archaeology is needed). Chunk loading is polled from the loading-screen state (`Chunks.startIfNeeded()`
/ `Chunks.update()` in `Game::updateLoading()`) so the pipeline must tolerate being driven incrementally, a
few chunks per call (`maxChunksPerIteration`), rather than run-to-completion.

**Collision.** `CollisionManager` (`src/collision/CollisionManager.hpp`) dispatches shape-pair collision
detection/response through type-erased tables keyed by `std::type_index` pairs
(`registerDetectionFn<A,B>`/`registerManifoldFn<A,B>`), auto-registering the reversed `(B,A)` pair too. New
collider shape pairs are added by registering functions into these tables (see `initDispatch()`), not by
editing a central switch statement. `ContactManifold`/`ContactResolver`/`KinematicResolver` resolve detected
contacts; `QuickHull` (vendored, public domain, `src/collision/QuickHull/`) generates convex hulls for
collision shapes.

**Rendering.** `DisplayManager` drives the frame each tick (`Display.render(currentTime)`), backed by
`ResourceManager`/`TextureManager`/`LightManager` and `Model`/`Mesh`/`InstancedMesh`/`InstancedModel`
abstractions. Shaders are raw GLSL files under `src/rendering/shaders/`, loaded at runtime by
`Display.loadShaders()` (no shader compilation step). ImGui (vendored, `external/imgui`) backs both debug UI
and the main menu/HUD (`src/gui/`, `src/hud/`).

**Precompiled header.** `src/core/PrecompiledHeader.hpp` is force-included on every translation unit via
`target_precompile_headers` in `CMakeLists.txt`. It pulls in GLFW/GLEW/glm/Eigen/assimp/rapidjson/stbi plus
most commonly used STL headers — new `.cpp`/`.hpp` files should generally rely on
`#include "core/PrecompiledHeader.hpp"` rather than re-including these directly.
