#version 420 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 inTexCoords;

out vec3 worldPosition;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

void main() {
    worldPosition = position.xyz;

    mat3 mat3Rot = mat3(view);
    mat4 rotView = mat4(mat3Rot);
    vec4 clipPos = projection * rotView * position;

    gl_Position = clipPos.xyww;
}