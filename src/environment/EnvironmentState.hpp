#pragma once

namespace EnvironmentState {
inline float globalMinHeight = std::numeric_limits<float>::max();
inline float globalMaxHeight = std::numeric_limits<float>::lowest();

inline glm::vec2 worldMin{};
inline glm::vec2 worldMax{};

// Bounds snapshotted when the world normal/tangent maps were last baked. Terrain shaders compute
// worldUV against these (not live worldMin/Max) so the maps never stretch as the window moves.
inline glm::vec2 normalMapMin{};
inline glm::vec2 normalMapMax{};
}  // namespace EnvironmentState