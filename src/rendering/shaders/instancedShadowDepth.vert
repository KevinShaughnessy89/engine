#version 420 core

layout(location = 0) in vec3 position;
layout(location = 2) in vec3 instancePosition;
layout(location = 3) in float size;
layout(location = 6) in float rotation;

uniform mat4 lightSpaceMatrix;

mat3 rotationY(float angle) {
    float c = cos(angle);
    float s = sin(angle);

    return mat3(c, 0, s, 0, 1, 0, -s, 0, c);
}

void main() {
    mat3 rot = rotationY(rotation);
    vec3 worldPosition = rot * (position * size) + instancePosition;

    gl_Position = lightSpaceMatrix * vec4(worldPosition, 1.0);
}
