#version 460 core

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D textureDiffuse;

void main() {
    // Mirror instancedModel.frag's cutout -- without this the G-buffer treats the whole
    // rectangular card as solid geometry, so SSAO self-occludes against the transparent parts of
    // overlapping/intersecting cards instead of the actual cutout leaf shape.
    if (texture(textureDiffuse, TexCoords).a < 0.5) discard;

    // Same backface flip as instancedModel.frag -- Normal is whatever the vertex shader
    // computed for the front face; without this, double-sided cards write the wrong (inward-
    // facing) normal for their backface pixels, which points ssao.frag's sample hemisphere into
    // the surface instead of away from it and self-occludes almost every sample.
    vec3 normal = gl_FrontFacing ? Normal : -Normal;

    gPosition = vec4(FragPos, 1.0);
    gNormal = vec4(normal, 1.0);
}
