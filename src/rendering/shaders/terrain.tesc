#version 460 core

layout(vertices = 3) out;

uniform vec3 viewPos;
float maxTessLevel = 16.0;
float minTessLevel = 1.0;
float tessDistanceNear = 10.0;
float tessDistanceFar = 50.0;

float tessLevelFromDistance(vec3 worldPosition) {
    float distance = distance(viewPos, worldPosition);
    float t = clamp((distance - tessDistanceNear) / (tessDistanceFar - tessDistanceNear), 0.0, 1.0);
    return mix(maxTessLevel, minTessLevel, t);
}

void main() {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    if (gl_InvocationID == 0) {
        vec3 p0 = gl_in[0].gl_Position.xyz;
        vec3 p1 = gl_in[1].gl_Position.xyz;
        vec3 p2 = gl_in[2].gl_Position.xyz;

        float l0 = tessLevelFromDistance((p1 + p2) / 2.0);
        float l1 = tessLevelFromDistance((p0 + p2) / 2.0);
        float l2 = tessLevelFromDistance((p0 + p1) / 2.0);

        gl_TessLevelOuter[0] = l0;
        gl_TessLevelOuter[1] = l1;
        gl_TessLevelOuter[2] = l2;
        gl_TessLevelInner[0] = (l0 + l1 + l2) / 3.0;
    }
}
