#version 420 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;

void main() {
    
    float weights[5] = float[](0.227027, 0.316216, 0.070270, 0.026216, 0.016216);
    vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
    vec3 result = texture(image, TexCoords).rgb * weights[0];
    for (int i = 1; i < 5; ++i) {
        vec2 offset = horizontal ? vec2(tex_offset.x * i, 0.0) : vec2(0.0, tex_offset.y * i);
        result += texture(image, TexCoords + offset).rgb * weights[i];
        result += texture(image, TexCoords - offset).rgb * weights[i];
    }
    FragColor = vec4(result, 1.0);
}