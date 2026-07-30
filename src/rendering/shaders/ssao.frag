#version 460 core

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseTexture;

in vec2 TexCoords;

layout(location = 0) out vec4 FragColor;

uniform float screenWidth;
uniform float screenHeight;
uniform vec3 samples[32];
uniform mat4 projection;

const float radius = 0.05;
const float bias = 0.30;
const float aoStrength = 1.5;

const int kernelSize = 32;

void main() {
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 rawNormal = texture(gNormal, TexCoords).xyz;
    // Pixels the G-buffer prepass never wrote to (e.g. sky, since Skybox/Sun are skipped there)
    // are left at the glClear value (0,0,0), and normalize() of a zero vector is NaN -- which the
    // blur pass's box filter then spreads into every neighboring pixel's sum, corrupting far more
    // of the frame than just the empty pixels themselves.
    if (dot(rawNormal, rawNormal) < 1e-6) {
        FragColor = vec4(1.0);
        return;
    }
    vec3 normal = normalize(rawNormal);

    vec2 noiseScale = vec2(screenWidth / 4.0, screenHeight / 4.0);
    vec3 noiseSample = texture(noiseTexture, noiseScale * TexCoords).xyz;

    vec3 tangent = normalize(noiseSample - normal * dot(noiseSample, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;

    for (int i = 0; i < kernelSize; ++i) {
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * radius;

        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(gPosition, offset.xy).z;

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));

        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(kernelSize));
    float ao = pow(occlusion, aoStrength);
    FragColor = vec4(ao, ao, ao, 1.0);
}