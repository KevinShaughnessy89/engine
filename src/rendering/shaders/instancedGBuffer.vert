#version 420 core

layout(location = 0) in vec3 position;
layout(location = 7) in vec3 normal;
layout(location = 1) in vec2 inTexCoords;
layout(location = 2) in vec3 instancePosition;
layout(location = 3) in float size;
layout(location = 6) in float rotation;

uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

mat3 rotationY(float angle) {
    float c = cos(angle);
    float s = sin(angle);

    return mat3(c, 0, s, 0, 1, 0, -s, 0, c);
}

void main() {
    mat3 rot = rotationY(rotation);
    vec3 worldPosition = rot * (position * size) + instancePosition;

    vec4 viewPos = view * vec4(worldPosition, 1.0);
    FragPos = viewPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(view)));
    Normal = normalMatrix * normal;
    TexCoords = inTexCoords;

    gl_Position = projection * viewPos;
}
