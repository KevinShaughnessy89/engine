#version 420 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec3 Tangent;
in vec3 Bitangent;

uniform vec3 tint;
uniform sampler2D textureDiffuse;
uniform sampler2D textureNormal;
uniform sampler2D textureSpecular;
uniform sampler2D textureShininess;
uniform sampler2D textureEmissive;
uniform samplerCube irradianceMap;
uniform sampler2D ssaoBlurTexture;

uniform bool useNormalMap;
uniform bool useSpecularMap;
uniform bool useShininessMap;
uniform bool useEmissiveMap;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 sunColor;
uniform float ambientStrength;
uniform float screenWidth;
uniform float screenHeight;

// Shininess maps are typically stored as a normalized gloss value; remap to a Blinn-Phong exponent.
const float minShininess = 4.0;
const float maxShininess = 256.0;

void main() {
    vec4 texColor = texture(textureDiffuse, TexCoords);
    if (texColor.a < 0.1) {
        discard;
    }
    vec3 albedo = texColor.rgb * tint;

    vec3 N = normalize(Normal);
    vec3 T = normalize(Tangent);
    vec3 B = normalize(Bitangent);
    if (!gl_FrontFacing) {
        N = -N;
        B = -B;
    }

    vec3 norm;
    if (useNormalMap) {
        mat3 TBN = mat3(T, B, N);
        vec3 normalSample = texture(textureNormal, TexCoords).rgb * 2.0 - 1.0;
        norm = normalize(TBN * normalSample);
    } else {
        norm = N;
    }

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 halfDir = normalize(viewDir + lightDir);

    float NdotL = max(dot(norm, lightDir), 0.0);

    vec3 specularColor = useSpecularMap ? texture(textureSpecular, TexCoords).rgb : vec3(0.0);
    float shininess = useShininessMap
                          ? mix(minShininess, maxShininess, texture(textureShininess, TexCoords).r)
                          : minShininess;
    float specAngle = max(dot(norm, halfDir), 0.0);
    float specFactor = NdotL > 0.0 ? pow(specAngle, shininess) : 0.0;
    vec3 specular = specularColor * specFactor * sunColor;

    vec3 diffuse = albedo * sunColor * NdotL;

    vec2 screenUV = vec2(gl_FragCoord.x / screenWidth, gl_FragCoord.y / screenHeight);
    float ssao = texture(ssaoBlurTexture, screenUV).r;
    vec3 irradiance = texture(irradianceMap, norm).rgb;
    vec3 ambient = irradiance * albedo * ambientStrength * ssao;

    vec3 emissive = useEmissiveMap ? texture(textureEmissive, TexCoords).rgb : vec3(0.0);

    vec3 result = ambient + diffuse + specular + emissive;
    FragColor = vec4(result, texColor.a);
}
