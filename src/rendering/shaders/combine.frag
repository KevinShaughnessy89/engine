#version 420 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D sceneTex;
uniform sampler2D bloomBlur;
uniform float time;

float exposure = 1.5;
float vignetteStrength = 0.5;
float aberrationAmount = 0.0003;
float saturation = 1.0;     // 1.0 = unchanged, <1 desaturate, >1 boost
float contrastGamma = 1.0;  // 1.0 = unchanged
float grainStrength = 0.02;

vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec2 uv = TexCoords;

    // 1. Chromatic aberration - re-sample scene+bloom per channel at offset UVs
    vec3 hdrColor;
    hdrColor.r = texture(sceneTex, uv + vec2(aberrationAmount, 0.0)).r +
                 texture(bloomBlur, uv + vec2(aberrationAmount, 0.0)).r;
    hdrColor.g = texture(sceneTex, uv).g + texture(bloomBlur, uv).g;
    hdrColor.b = texture(sceneTex, uv - vec2(aberrationAmount, 0.0)).b +
                 texture(bloomBlur, uv - vec2(aberrationAmount, 0.0)).b;

    // 2. Vignette - darken edges
    float vignette = 1.0 - dot(uv - 0.5, uv - 0.5) * vignetteStrength;
    hdrColor *= vignette;

    // 3. Tone map (HDR -> LDR)
    vec3 color = ACESFilm(hdrColor * exposure);

    // // 4. Contrast + saturation - operates on the now-LDR color
    // color = mix(vec3(dot(color, vec3(0.299, 0.587, 0.114))), color, saturation);
    // color = pow(color, vec3(contrastGamma));

    // 5. Film grain - additive noise, applied last
    // float grain =
    //     (fract(sin(dot(uv + time, vec2(12.9898, 78.233))) * 43758.5453) - 0.5) * grainStrength;
    // color += grain;

    FragColor = vec4(color, 1.0);
}