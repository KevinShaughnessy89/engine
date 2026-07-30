#version 420 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

void main() {
    // TCS/TES now own the model/view/projection transform (applied after displacement), so
    // gl_Position stays in object space here (== world space for terrain, whose model matrix is
    // always identity) instead of being pre-multiplied like a non-tessellated pipeline would.
    gl_Position = position;
}
