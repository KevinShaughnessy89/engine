#version 420 core
out vec4 FragColor;

in vec3 worldPosition;

uniform vec3 horizonColor;
uniform vec3 topColor;
uniform vec3 bottomColor;
uniform vec3 sunPosition;

void main() {
    vec3 sunDirection = normalize(sunPosition);
    vec3 direction = normalize(worldPosition);
    float elevation = direction.y;

    float t = clamp(elevation, 0.0, 1.0);

    vec3 skyColor;
    if (t < 0.3) {
        skyColor = mix(bottomColor, horizonColor, t / 0.3);
    } else {
        skyColor = mix(horizonColor, topColor, (t - 0.3) / 0.7);
    }

    vec3 sunGlowColor = vec3(1.0, 0.6, 0.3);  // Warm yellowish glow
    float sunAmount = max(dot(direction, sunDirection), 0.0);
    float glowSharpness = 100.0;  // Adjust this value to control the sharpness of the glow
    float sunGlow = pow(sunAmount, glowSharpness);

    skyColor += sunGlow * sunGlowColor;

    FragColor = vec4(skyColor, 1.0);
}