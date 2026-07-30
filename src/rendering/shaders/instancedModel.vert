#version 420 core

layout(location = 0) in vec3 position;
layout(location = 7) in vec3 normal;
layout(location = 8) in vec3 tangent;
layout(location = 9) in vec3 bitangent;
layout(location = 1) in vec2 inTexCoords;
layout(location = 2) in vec3 instancePosition;
layout(location = 3) in float size;
layout(location = 4) in vec4 color;
layout(location = 5) in vec2 skew;
layout(location = 6) in float rotation;

uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;
out vec3 Normal;
out vec3 Tangent;
out vec3 Bitangent;
out vec3 FragPos;

mat3 rotationY(float angle) {
    float c = cos(angle);
    float s = sin(angle);

    return mat3(c, 0, s, 0, 1, 0, -s, 0, c);
}

void main() {
    mat3 rot = rotationY(rotation);
    vec3 worldPosition = rot * (position * size) + instancePosition;

    Tangent = normalize(rot * tangent);
    Bitangent = normalize(rot * bitangent);
    Normal = normalize(rot * normal);

    TexCoords = inTexCoords;
    FragPos = worldPosition;
    gl_Position = projection * view * vec4(worldPosition, 1.0);
}
