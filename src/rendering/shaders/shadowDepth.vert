#version 420 core

layout(location = 0) in vec4 position;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main() {
    gl_Position = lightSpaceMatrix * model * position;
}
