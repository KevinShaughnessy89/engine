#version 420 core

out vec4 FragColor;
in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec3 Tangent;
in vec3 Bitangent;

uniform sampler2D textureDiffuse;
uniform sampler2D textureNormal;
uniform sampler2D textureAlpha;
uniform samplerCube irradianceMap;
uniform bool useNormalMap;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 skyColorAverage;
uniform vec3 sunColor;
uniform float ambientStrength;
uniform sampler2D ssaoBlurTexture;
uniform float screenWidth;
uniform float screenHeight;
uniform bool useAlphaMap;

const float roughness = 0.5;  // example value, adjust as needed
const float metalness = 0.0;  // example value, adjust as needed

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
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    vec4 texColor = texture(textureDiffuse, TexCoords);

    float alpha = 1.0;
    if (useAlphaMap) {
        alpha = texture(textureAlpha, TexCoords).r;
    } else {
        if (texColor.a < 0.1) {
            discard;
        }
    }

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(lightPos - FragPos);

    vec3 N = normalize(Normal);  // local copy — this is now yours to modify
    vec3 T = normalize(Tangent);
    vec3 B = normalize(Bitangent);
    vec3 norm;
    vec3 H = normalize(viewDir + lightDir);
    // Double-sided geometry (e.g. cross-card foliage) needs this regardless of whether a normal
    // map is present, or the backface just reuses the frontface's normal and reads as unlit.
    if (!gl_FrontFacing) {
        N = -N;
        B = -B;  // flip bitangent too — keeps TBN a proper right-handed basis
    }
    if (useNormalMap) {
        mat3 TBN = mat3(T, B, N);
        vec3 normalSample = texture(textureNormal, TexCoords).rgb * 2.0 - 1.0;
        norm = normalize(TBN * normalSample);
    } else {
        norm = N;
    }

    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 albedo = texColor.rgb;

    float NdotL = max(dot(norm, lightDir), 0.0);
    float NdotV = max(dot(norm, viewDir), 0.0);

    float D = D_GGX(norm, H, roughness);
    float G = G_Smith(norm, viewDir, lightDir, roughness);

    vec3 F0 =
        mix(vec3(0.04), albedo,
            metalness);  // 0.04 ≈ typical dielectric reflectance; metals use their albedo as F0
    vec3 F = F_Schlick(max(dot(H, viewDir), 0.0), F0);

    vec3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.0001);
    vec2 screenUV = vec2(gl_FragCoord.x / screenWidth, gl_FragCoord.y / screenHeight);
    float ssao = texture(ssaoBlurTexture, screenUV).r;

    vec3 kD = (vec3(1.0) - F) * (1.0 - metalness);
    vec3 diffuse = kD * albedo / 3.14159265;

    vec3 radiance = sunColor;  // your existing light color
    vec3 Lo = (diffuse + specular) * radiance * NdotL;

    vec3 irradiance = texture(irradianceMap, norm).rgb;
    vec3 diffuseAmbient = irradiance * albedo * ambientStrength * (1.0 - metalness) * ssao;

    vec3 result = diffuseAmbient + Lo;
    FragColor = vec4(result, alpha);
}
