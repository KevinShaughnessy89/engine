#version 420 core
#extension GL_ARB_texture_query_levels : require

layout(std140, binding = 0) uniform CascadeData {
    mat4 lightSpaceMatrices[4];  // bump alongside ShadowState::shadowCascadeCount
    vec4 cascadeSplits;
};

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

const int DEFAULT_LAYER = 0;
const int ROCKY_INCLINE = 1;
const int MOSSY_LAYER = 2;

in vec3 teNormal;
in vec3 teTangent;
in vec3 teBitangent;
in vec3 teWorldPos;
in float teViewSpaceDepth;

uniform vec3 viewPos;
uniform sampler2DArray textureNormalArray;
uniform sampler2DArray textureDiffuseArray;
uniform sampler2D textureRoughness;

uniform sampler2D ssaoBlurTexture;
uniform sampler2DArray shadowMapArray;
uniform samplerCube irradianceMap;
uniform float globalMinHeight;
uniform float globalMaxHeight;
uniform vec3 sunPosition;
uniform float screenWidth;
uniform float screenHeight;
uniform vec3 lightColor;
uniform float ambientStrength;

vec3 lightDir = normalize(sunPosition);

float tileScale = 4.0;
// Continuous (non-atlas) world-space tiling coordinate for the repeating detail textures --
// deliberately NOT chunk-relative like teAtlasUV, so the tiling pattern doesn't reset/seam at
// chunk borders. Matches the uniform terrain.tese and compute.comp use for chunk sizing.
uniform float chunkWorldSize;  // TerrainConfig::gridSize, set from C++ each frame

const float metalness = 0.0;
float roughness;

// PBR
float D_GGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (3.14159265359 * denom * denom);
}

float G_SchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = G_SchlickGGX(NdotV, roughness);
    float ggx1 = G_SchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 F_Schlick(float cosTheta, vec3 F0) {
    vec3 Fmax = max(vec3(1.0 - roughness), F0);  // no "F0 +" prefix
    return F0 + (Fmax - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 getPBRLighting(vec3 normal, vec3 viewDir, vec3 albedo) {
    vec3 H = normalize(viewDir + lightDir);

    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);

    float D = D_GGX(normal, H, roughness);
    float G = G_Smith(normal, viewDir, lightDir, roughness);

    vec3 F0 =
        mix(vec3(0.04), albedo,
            metalness);  // 0.04 ≈ typical dielectric reflectance; metals use their albedo as F0
    vec3 F = F_Schlick(max(dot(H, viewDir), 0.0), F0);

    vec3 specular = (D * G * F) /
                    (4.0 * NdotV * NdotL + 0.001);  // add small epsilon to prevent division by zero

    vec3 kD = (vec3(1.0) - F) * (1.0 - metalness);
    vec3 diffuse = kD * albedo / 3.14159265;

    return (diffuse + specular) * lightColor * NdotL;
}

// SHADOW

float calculateShadow(vec3 fragPosWorld, vec3 normal) {
    // cascadeSplits holds each cascade's far-edge view-space depth (cascadeSplits[3] ==
    // shadowFarPlane); only the first 3 boundaries need checking since anything past
    // cascadeSplits[2] falls into the last cascade (index 3) by elimination.
    int cascadeIndex = 0;
    for (int i = 0; i < 3; ++i) {
        if (teViewSpaceDepth > cascadeSplits[i]) cascadeIndex = i + 1;
    }

    vec4 lightSpacePos = lightSpaceMatrices[cascadeIndex] * vec4(fragPosWorld, 1.0);

    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Outside the ortho box, projCoords.z isn't bounded to [0,1] the way a real sampled depth
    // is (texture reads always return a value in-range, real data or the 1.0 border) -- without
    // clamping, currentDepth can exceed every possible pcfDepth, so every out-of-box fragment
    // would register as "in shadow" instead of inheriting the border's "not in shadow" default.
    float currentDepth = clamp(projCoords.z, 0.0, 1.0);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.003);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMapArray, 0).xy);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth =
                texture(shadowMapArray, vec3(projCoords.xy + vec2(x, y) * texelSize, cascadeIndex))
                    .r;
            shadow += (currentDepth - bias > pcfDepth) ? 0.2 : 1.0;
        }
    }
    return shadow / 9.0;  // averaged over the 3x3 neighborhood
}

// ALBEDO

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

// Rotates the fractional (per-tile) part of uv by a random angle chosen per integer tile ID,
// so adjacent repeats of the tiling texture no longer line up identically.
vec3 sampleRotatedTile(sampler2D tex, vec2 uv) {
    vec2 tileId = floor(uv);
    float angle = hash21(tileId) * 6.2831853;
    float s = sin(angle);
    float c = cos(angle);
    vec2 centered = fract(uv) - 0.5;
    vec2 rotated = vec2(centered.x * c - centered.y * s, centered.x * s + centered.y * c) + 0.5;
    return texture(tex, rotated).rgb;
}

vec3 getAlbedoColor(vec2 worldUV, vec3 geometryNormal, inout vec3 mapNormal) {
    float slope = 1.0 - dot(geometryNormal, vec3(0.0, 1.0, 0.0));

    vec2 tiledUV = worldUV * tileScale;
    vec2 tiledUVFar = worldUV * (tileScale * 0.10);  // <-- different frequency

    const float centerDefault = 0.0;
    const float centerRocky = 0.30;

    int layerA = DEFAULT_LAYER, layerB = MOSSY_LAYER;
    float t;
    if (slope >= centerRocky) {
        layerA = DEFAULT_LAYER;
        layerB = MOSSY_LAYER;
        t = smoothstep(0.0, 1.0, slope);
    }

    vec3 red = vec3(1.0, 0.0, 0.0);
    vec3 blue = vec3(0.0, 0.0, 1.0);

    vec3 albedoNearA = texture(textureDiffuseArray, vec3(tiledUV, layerA)).rgb;
    vec3 albedoNearB = texture(textureDiffuseArray, vec3(tiledUV, layerB)).rgb;

    vec3 albedoFarA = texture(textureDiffuseArray, vec3(tiledUVFar, layerA)).rgb;
    vec3 albedoFarB = texture(textureDiffuseArray, vec3(tiledUVFar, layerB)).rgb;

    vec3 normalA = texture(textureNormalArray, vec3(tiledUV, layerA)).rgb * 2.0 - 1.0;
    vec3 normalB = texture(textureNormalArray, vec3(tiledUV, layerB)).rgb * 2.0 - 1.0;

    vec3 albedoNear = mix(albedoNearA, albedoNearB, t);
    vec3 albedoFar = mix(albedoFarA, albedoFarB, t);
    mapNormal = normalize(mix(normalA, normalB, t));

    float mipLevel = textureQueryLod(textureDiffuseArray, tiledUV).x;
    float maxMip = float(textureQueryLevels(textureDiffuseArray) - 1);
    float blendFactor = clamp(mipLevel / maxMip, 0.0, 1.0);

    vec3 dryTint = vec3(0.55, 0.45, 0.35);  // exposed soil/rock tint
    float tintFactor = 1.4;
    // vec3 dryTint = vec3(1.0, 0.0, 0.0);
    vec3 mixColor = mix(albedoNear, albedoFar, blendFactor);
    return mix(mixColor, mixColor * dryTint * tintFactor, slope);
}

// Debug: bypasses all lighting/texture blending and outputs raw slope as grayscale (0 = flat,
// 1 = vertical) so the underlying slope signal can be inspected directly, decoupled from whatever
// the blend logic does with it. Flip to false and relaunch to go back to normal shading.
const bool DEBUG_SHOW_SLOPE = false;
// Debug: bypasses lighting/shadow/ambient entirely and outputs raw getAlbedoColor() output --
// isolates whether the near/far distance blend is going black at distance, independent of
// lighting, shadow cascades, or SSAO. Flip to false and relaunch to go back to normal shading.
const bool DEBUG_SHOW_ALBEDO = false;

void main() {
    vec2 tilingUV = teWorldPos.xz / chunkWorldSize;
    roughness = texture(textureRoughness, tilingUV * tileScale).r;
    vec2 screenUV = vec2(gl_FragCoord.xy / vec2(screenWidth, screenHeight));

    // Re-normalize: these are linearly interpolated across the triangle from per-vertex values,
    // which doesn't preserve unit length even though each vertex's was already normalized.
    vec3 geometryNormal = normalize(teNormal);

    if (DEBUG_SHOW_SLOPE) {
        float debugSlope = 1.0 - dot(geometryNormal, vec3(0.0, 1.0, 0.0));
        FragColor = vec4(vec3(debugSlope), 1.0);
        BrightColor = vec4(0.0);
        return;
    }

    vec3 T = normalize(teTangent);
    vec3 B = normalize(teBitangent);

    mat3 TBN = mat3(T, B, geometryNormal);

    vec3 mapNormal;
    vec3 baseColor = getAlbedoColor(tilingUV, geometryNormal, mapNormal);

    if (DEBUG_SHOW_ALBEDO) {
        FragColor = vec4(baseColor, 1.0);
        BrightColor = vec4(0.0);
        return;
    }

    mapNormal = TBN * mapNormal;
    vec3 blendedNormal = mix(mapNormal, geometryNormal, 0.5);
    float NdotL = max(dot(blendedNormal, lightDir), 0.0);

    vec3 viewDir = normalize(viewPos - teWorldPos);
    vec3 Lo = getPBRLighting(blendedNormal, viewDir, baseColor);
    float ssao = texture(ssaoBlurTexture, screenUV).r;
    // commenting this out because it produces a very strong rim light effect that looks bad
    //  float fresnel = pow(1.0 - max(dot(viewDir, blendedNormal), 0.0), 5.0);
    //  vec3 rimLight =  lightColor * fresnel * 0.05 * NdotL;
    vec3 irradiance = texture(irradianceMap, blendedNormal).rgb;
    vec3 diffuseAmbient = irradiance * baseColor * ambientStrength * (1.0 - metalness) * ssao;

    float shadow = calculateShadow(teWorldPos, geometryNormal);
    Lo *= shadow;

    vec3 result = Lo + diffuseAmbient;

    FragColor = vec4(result, 1.0);  // 5 (F_Schlick result, vec3)
    BrightColor = vec4(0.0);
}