#version 460 core

layout(std430, binding = 0) readonly buffer SkinBuffer {
    mat4 skinMatrices[];
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;
layout(location = 5) in ivec4 boneIDs;
layout(location = 6) in vec4 boneWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int skinSlot;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 Bitangent;

const int MAX_BONES_PER_ENTITY = 100;

void main() {
    mat4 skinMatrix = boneWeights.x * skinMatrices[skinSlot * MAX_BONES_PER_ENTITY + boneIDs.x] +
                      boneWeights.y * skinMatrices[skinSlot * MAX_BONES_PER_ENTITY + boneIDs.y] +
                      boneWeights.z * skinMatrices[skinSlot * MAX_BONES_PER_ENTITY + boneIDs.z] +
                      boneWeights.w * skinMatrices[skinSlot * MAX_BONES_PER_ENTITY + boneIDs.w];

    vec4 skinnedPosition = skinMatrix * position;
    vec4 worldPos = model * skinnedPosition;
    gl_Position = projection * view * worldPos;

    mat3 normalMatrix = mat3(model) * mat3(skinMatrix);
    FragPos = worldPos.xyz;
    Normal = normalize(normalMatrix * normal);
    Tangent = normalize(normalMatrix * tangent);
    Bitangent = normalize(normalMatrix * bitangent);
    TexCoords = inTexCoords;
}
