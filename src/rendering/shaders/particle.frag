#version 420 core

in vec4 particleColor; // could pass in life or velocity
in vec2 TexCoords;      // optional if you use a noise/gradient texture

out vec4 FragColor;

void main()
{
    vec3 baseColor = vec3(0.5, 0.5, 1.0);

    // Create a vertical streak by fading alpha from top to bottom
    float alpha = 0.0;

    // You can use smoothstep to make the fade smooth
    float streakStart = 0.0;   // bottom of streak
    float streakEnd = 1.0;     // top of streak
    alpha = smoothstep(streakStart, streakEnd, TexCoords.y);

    // Optional: make edges softer horizontally
    float distX = abs(TexCoords.x - 0.5); // distance from center horizontally
    alpha *= smoothstep(0.5, 0.0, distX);

    // Scale alpha to desired opacity
    alpha *= 0.9;

    FragColor = vec4(baseColor, alpha);
}