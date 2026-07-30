#version 420 core

layout(location = 0) out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D textureDiffuse;

void main() {
    vec4 texColor = texture(textureDiffuse, TexCoords);
    if (texColor.a < 0.5) discard;

    FragColor = vec4(texColor.rgb, 1.0);
}
