#pragma once

// Tag for entities created this frame. Physics and collision skip tagged
// entities so the fixed-rate worker thread never touches a freshly created
// entity mid-frame; the main thread clears all tags each frame in
// Game::updateWorldState before new spawns are processed.
struct JustCreated {};
