#version 450

layout(location = 0) in float fragViewDist;
layout(location = 1) in float fragViewX;
layout(location = 0) out vec4 outColor;

layout(set = 3, binding = 0) uniform FragUBO {
    vec4  color;
    float fogStrength;
    float fogStart;
    float fogEnd;
    float sideStart;
    float sideEnd;
    float _pad0;
    float _pad1;
    float _pad2;
} u;

void main() {
    float t_depth = clamp((fragViewDist - u.fogStart) / (u.fogEnd - u.fogStart), 0.0, 1.0);
    float t_side  = clamp((abs(fragViewX) - u.sideStart) / (u.sideEnd - u.sideStart), 0.0, 1.0);
    float t = 1.0 - (1.0 - t_depth) * (1.0 - t_side);
    outColor = vec4(u.color.rgb * (1.0 - t * u.fogStrength), u.color.a);
}
