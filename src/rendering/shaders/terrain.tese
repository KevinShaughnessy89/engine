#version 460 core

layout(triangles, fractional_odd_spacing, ccw) in;

out vec3 teNormal;
out vec3 teWorldPos;
out float teViewSpaceDepth;
out vec3 teTangent;
out vec3 teBitangent;

// XZ components of the precomputed per-texel normal (Y reconstructed below); baked on the CPU by
// TerrainMeshGenerator::calculateNormals using the same Sobel gradient this shader used to compute
// per-fragment, now a single texture fetch instead of an 8-tap kernel.
uniform sampler2D normalMapBuffer;
uniform sampler2D textureDisplacement;
float displacementScale = 0.40;  // Adjust this value to control the height of the displacement
float displacementMidLevel = 0.5;
float tileScale = 4.0;  // remember to sync with fragment shader

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

    // localPos.y is already the correct baked macro terrain height (TerrainMeshGenerator::
    // generateVertices writes it straight into the vertex buffer) -- normalMapBuffer only feeds
    // the smooth per-vertex normal below, since linear (barycentric) interpolation of p0/p1/p2
    // alone would give a faceted normal. It never moves the vertex -- only the artist-authored
    // textureDisplacement detail layer does that, below.
    vec4 worldPosition = model * vec4(localPos, 1.0);
    vec2 worldXZ = worldPosition.xz;

    vec2 uv = worldXZ / (worldTexelSize * normalMapBufferSize);

    vec2 xz = texture(normalMapBuffer, uv).rg;
    float ny = sqrt(max(0.0, 1.0 - xz.x * xz.x - xz.y * xz.y));
    vec3 normal = vec3(xz.x, ny, xz.y);

    // tangent/bitangent are pure functions of normal (no independent data), same construction as
    // the old Sobel path: tangent = normalize(vec3(1, Gx*scale, 0)) where Gx*scale == -normal.x/
    // normal.y by construction of normal above, then Gram-Schmidt against normal.
    vec3 tangent = normalize(vec3(1.0, -normal.x / normal.y, 0.0));
    tangent = normalize(tangent - normal * dot(tangent, normal));

    vec3 bitangent = cross(normal, tangent);

    // Artist-authored detail displacement, tiled continuously in world space (not chunk-relative)
    // so it doesn't reset/seam at chunk borders -- same tiling convention terrain.frag uses for
    // its own detail textures. Applied along the normal so bumps follow the slope.
    vec2 tilingUV = worldXZ / chunkWorldSize;
    float detailHeight = texture(textureDisplacement, tilingUV * tileScale).r;
    worldPosition.xyz += normal * ((detailHeight - displacementMidLevel) * displacementScale);

    teNormal = normal;
    teWorldPos = worldPosition.xyz;
    teViewSpaceDepth = -(view * worldPosition).z;
    teTangent = tangent;
    teBitangent = bitangent;
    gl_Position = projection * view * worldPosition;
}
