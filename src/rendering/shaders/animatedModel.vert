#version 460 core

layout(std430, binding = 0) readonly buffer SkinBuffer {
    mat4 skinMatrices[];
}

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 5) in ivec4 boneIDs;
layout(location = 6) in vec4 boneWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int skinSlot;

out vec2 TexCoords;

const int MAX_BONES_PER_ENTITY = 100;

void main() {
    mat4 skinMatrix = boneWeights.x * skinMatrices[skinSlot * MAX_BONES_PER_ENTITY + boneIDs.x] +
                      boneWeights.y * skinMatrices[skinSlot * MAX_BONES_PER_ENTITY + boneIDs.y] +
                      boneWeights.z * skinMatrices[skinSlot * MAX_BONES_PER_ENTITY + boneIDs.z] +
                      boneWeights.w * skinMatrices[skinSlot * MAX_BONES_PER_ENTITY + boneIDs.w];

    vec4 skinnedPosition = skinMatrix * position;

    gl_Position = projection * view * model * skinnedPosition;
    TexCoords = inTexCoords;
}