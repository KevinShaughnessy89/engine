#version 460 core

layout(triangles, fractional_odd_spacing, ccw) in;

out vec3 FragPos;
out vec3 Normal;

uniform sampler2D normalMapBuffer;
uniform sampler2D textureDisplacement;
// Must match terrain.tese's values -- this pass feeds SSAO/depth, so its displaced geometry has
// to agree with what the main color pass actually renders, or AO reads as offset from the surface.
float displacementScale = 0.40;
float displacementMidLevel = 0.5;
float tileScale = 720.0;  // remember to sync with terrain.tese/frag

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float worldTexelSize;       // world units per clipmap texel: gridSize / triangleCount
uniform float normalMapBufferSize;  // normalMapBuffer's actual texel dimension (its bufferSize)
uniform float chunkWorldSize;       // TerrainConfig::gridSize, set from C++ each frame

void main() {
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;

    vec3 localPos = gl_TessCoord.x * p0 + gl_TessCoord.y * p1 + gl_TessCoord.z * p2;

    // localPos.y is already the correct baked terrain height -- normalMapBuffer only feeds the
    // precomputed normal below; textureDisplacement is what actually moves the vertex.
    vec4 worldPosition = model * vec4(localPos, 1.0);
    vec2 worldXZ = worldPosition.xz;
    vec2 uv = worldXZ / (worldTexelSize * normalMapBufferSize);

    // Same precomputed normal as terrain.tese -- see that file for the derivation.
    vec2 xz = texture(normalMapBuffer, uv).rg;
    float ny = sqrt(max(0.0, 1.0 - xz.x * xz.x - xz.y * xz.y));
    vec3 geometricNormal = vec3(xz.x, ny, xz.y);

    vec2 tilingUV = worldXZ / chunkWorldSize;
    float detailHeight = texture(textureDisplacement, tilingUV * tileScale).r;
    worldPosition.xyz +=
        geometricNormal * ((detailHeight - displacementMidLevel) * displacementScale);

    vec4 viewPosition = view * worldPosition;

    FragPos = viewPosition.xyz;
    Normal = normalize(mat3(transpose(inverse(view * model))) * geometricNormal);

    gl_Position = projection * viewPosition;
}
