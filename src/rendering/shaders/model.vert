#version 420 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 Bitangent;

void main() {
    vec4 worldPos = model * position;
    gl_Position = projection * view * worldPos;

    mat3 normalMatrix = mat3(model);
    FragPos = worldPos.xyz;
    Normal = normalize(normalMatrix * normal);
    Tangent = normalize(normalMatrix * tangent);
    Bitangent = normalize(normalMatrix * bitangent);
    TexCoords = inTexCoords;
}
