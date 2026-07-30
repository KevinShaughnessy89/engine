#version 420 core

layout(location = 0) in vec3 position;  // quad-local, already scaled to boundingRadius
layout(location = 1) in vec2 inTexCoords;
layout(location = 2) in vec3 instancePosition;
layout(location = 3) in float size;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

// gbuffer.frag's contract (view-space position/normal), so it can be reused unchanged here.
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
flat out vec3 InstancePos;

void main() {
    // Same cylindrical billboard as imposterModel.vert -- the card has no real surface
    // geometry, so the G-buffer pass has to reconstruct the same camera-facing orientation the
    // forward pass draws, or its position/normal won't line up with what's actually on screen.
    vec3 toCamera = viewPos - instancePosition;
    toCamera.y = 0.0;
    float distXZ = length(toCamera);
    vec3 forward = distXZ > 0.0001 ? toCamera / distXZ : vec3(0.0, 0.0, 1.0);

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, forward));

    vec3 worldPosition = instancePosition + right * position.x * size + up * position.y * size;
    vec4 viewPosition = view * vec4(worldPosition, 1.0);

    FragPos = viewPosition.xyz;
    // view's rotational part is orthonormal, so transforming the direction directly is fine (no
    // inverse-transpose needed the way a non-uniformly scaled normal would require).
    Normal = normalize(mat3(view) * forward);
    TexCoords = inTexCoords;
    InstancePos = instancePosition;

    gl_Position = projection * viewPosition;
}
