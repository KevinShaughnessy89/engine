#version 420 core
layout (location = 0) in vec4 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    mat3 mat3Rot = mat3(view);
    mat4 rotView = mat4(mat3Rot);
    gl_Position = projection * rotView * model * position;
}