#version 460 core

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
flat in vec3 InstancePos;

uniform sampler2D treeAtlasTexture;
uniform vec3 viewPos;
uniform int gridRes;

// Same octahedral tile lookup as imposterModel.frag -- the atlas is packed per view
// direction, so the G-buffer has to pick the same tile the forward pass actually shades to know
// which pixels are real leaf vs. transparent cutout.
vec2 octEncode(vec3 n) {
    n = normalize(n);
    float l1norm = abs(n.x) + abs(n.y) + abs(n.z);
    return n.xy / l1norm;
}

void main() {
    vec3 viewDir = normalize(viewPos - InstancePos);
    vec2 oct = octEncode(viewDir);
    vec2 gridUV = oct * 0.5 + 0.5;

    ivec2 tile = clamp(ivec2(floor(gridUV * float(gridRes))), ivec2(0), ivec2(gridRes - 1));
    vec2 tileOrigin = vec2(tile) / float(gridRes);
    vec2 finalUV = tileOrigin + TexCoords / float(gridRes);

    if (texture(treeAtlasTexture, finalUV).a < 0.5) discard;

    gPosition = vec4(FragPos, 1.0);
    gNormal = vec4(Normal, 1.0);
}
