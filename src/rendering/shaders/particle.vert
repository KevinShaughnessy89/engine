#version 420 core
layout(location = 0) in vec4 quadOffset;
layout(location = 1) in vec2 inTextureCoords;
layout(location = 2) in vec3 instancedOffset;
layout(location = 3) in float size;
layout(location = 4) in vec4 color;
layout(location = 5) in vec2 skew;       // x/y skew factor
layout(location = 6) in float rotation;  // rotation around camera forward

out vec4 particleColor;
out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 viewPos;

void main() {
    particleColor = color;
    TexCoords = inTextureCoords;
    vec3 particleCenter = instancedOffset;
    vec3 cameraForward = normalize(viewPos - particleCenter);
    vec3 cameraUp = vec3(0.0, 1.0, 0.0);
    vec3 cameraRight = normalize(cross(cameraUp, cameraForward));
    cameraUp = cross(cameraForward, cameraRight);

    vec2 offset = quadOffset.xy;  // base quad [-0.5,0.5]
    offset.x = offset.x * skew.x;
    offset.y = offset.y * skew.y;

    // optionally rotate
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    vec2 rotatedOffset = vec2(offset.x * cosR - offset.y * sinR, offset.x * sinR + offset.y * cosR);

    // final vertex position
    vec3 vertexPosition =
        particleCenter + cameraRight * rotatedOffset.x * size + cameraUp * rotatedOffset.y * size;

    gl_Position = projection * view * vec4(vertexPosition, 1.0);
}