#version 420 core

out vec4 FragColor;
in vec2 TexCoords;

const float checkerScale = 10.0;
const vec3 checkerColorA = vec3(1.0);
const vec3 checkerColorB = vec3(0.2);

void main() {
    vec2 checkerCoords = floor(TexCoords * checkerScale);
    float checker = mod(checkerCoords.x + checkerCoords.y, 2.0);
    FragColor = vec4(mix(checkerColorA, checkerColorB, checker), 1.0);
}
