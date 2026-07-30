#version 420 core

layout(location = 0) out vec4 FragColor;      // Final scene color
layout(location = 1) out vec4 BrightColor;    // Only emissive parts

uniform vec3 sunColor;
uniform float intensity;
float bloomMultiplier = 2.0;

void main() {
    // Keep the visible disc at its true color -- multiplying by intensity here blows every
    // channel out to the same near-white plateau once combine.frag's tonemap saturates,
    // which is what was reading as "grey/uncolored" regardless of the actual sunColor hue.
    FragColor = vec4(sunColor, 1.0);
    // Bright extraction for bloom: intensity drives glow strength instead.
    float brightness = dot(sunColor, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 0.1) { // using same threshold as your extractor
        float bloomAmount = max(0.0, brightness - 0.1) * intensity;
        BrightColor = vec4(sunColor * bloomAmount * bloomMultiplier, 1.0);
    }
    else
        BrightColor = vec4(0.0);
}