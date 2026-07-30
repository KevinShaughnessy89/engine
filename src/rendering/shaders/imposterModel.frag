#version 420 core

out vec4 FragColor;

in vec2 TexCoords;
flat in vec3 InstancePos;
in vec3 TintColor;
in vec3 Normal;

uniform sampler2D treeAtlasTexture;
uniform samplerCube irradianceMap;
uniform vec3 viewPos;
uniform vec3 sunPosition;
uniform vec3 sunColor;
uniform float ambientStrength;
uniform int gridRes;

vec2 octEncode(vec3 n) {
    n = normalize(n);
    float l1norm = abs(n.x) + abs(n.y) + abs(n.z);
    return n.xy / l1norm;  // [-1, 1]
}

vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    vec3 viewDir = normalize(viewPos - InstancePos);

    vec2 oct = octEncode(viewDir);  // [-1, 1]
    vec2 gridUV = oct * 0.5 + 0.5;  // [0, 1]

    // Snap to the single nearest baked tile (no cross-tile blending).
    ivec2 tile = clamp(ivec2(floor(gridUV * float(gridRes))), ivec2(0), ivec2(gridRes - 1));

    vec2 tileOrigin = vec2(tile) / float(gridRes);
    vec2 finalUV = tileOrigin + TexCoords / float(gridRes);

    vec4 texColor = texture(treeAtlasTexture, finalUV);
    if (texColor.a < 0.5) discard;

    // Same ambient + direct-light combination as terrain.frag/instancedModel.frag,
    // minus the specular term -- the billboard's normal is only a camera-facing approximation, so
    // a GGX highlight on it would just look like a wandering hotspot rather than real geometry.
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(sunPosition);
    vec3 H = normalize(viewDir + lightDir);
    vec3 irradiance = texture(irradianceMap, norm).rgb;
    float NdotL = max(dot(norm, lightDir), 0.0);

    vec3 albedo = texColor.rgb * TintColor;

    // Same Lambertian normalization (kD, /pi) as instancedModel.frag's diffuse term, so the
    // two match in brightness under identical sun/ambient conditions.
    vec3 F = F_Schlick(max(dot(H, viewDir), 0.0), vec3(0.04));
    vec3 kD = vec3(1.0) - F;
    vec3 diffuse = kD * albedo / 3.14159265;

    vec3 ambient = irradiance * albedo * ambientStrength;
    vec3 direct = diffuse * sunColor * NdotL;

    FragColor = vec4(ambient + direct, texColor.a);
}
