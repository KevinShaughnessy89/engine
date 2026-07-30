#version 420 core

layout(location = 0) in vec3 position;  // quad-local, already scaled to boundingRadius
layout(location = 1) in vec2 inTexCoords;
layout(location = 2) in vec3 instancePosition;
layout(location = 3) in float size;
layout(location = 4) in vec3 color;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

out vec2 TexCoords;
flat out vec3 InstancePos;
out vec3 TintColor;
// The card has no real surface normal (it's a flat billboard). The camera-facing direction is
// used for the card's orientation, but a real canopy's leaf normals are scattered across a
// hemisphere with a strong upward bias, so lighting off the pure-horizontal facing direction
// alone reads noticeably darker than the real tree under the same sky/sun. Tilting the lighting
// normal upward approximates that canopy average.
out vec3 Normal;

void main() {
    // Cylindrical billboard: rotate only around world-up so the card stays upright.
    vec3 toCamera = viewPos - instancePosition;
    toCamera.y = 0.0;
    float distXZ = length(toCamera);
    // Guard the degenerate case where the camera sits directly above/below the instance.
    vec3 forward = distXZ > 0.0001 ? toCamera / distXZ : vec3(0.0, 0.0, 1.0);

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, forward));

    vec3 worldPosition = instancePosition + right * position.x * size + up * position.y * size;

    TexCoords = inTexCoords;
    InstancePos = instancePosition;
    TintColor = color;
    Normal = normalize(mix(forward, up, 0.5));

    gl_Position = projection * view * vec4(worldPosition, 1.0);
}
