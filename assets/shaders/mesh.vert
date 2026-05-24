#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out float fragViewDist;
layout(location = 1) out float fragViewX;

layout(set = 1, binding = 0) uniform UBO {
    mat4 modelView;
    mat4 proj;
} ubo;

void main() {
    vec4 viewPos = ubo.modelView * vec4(inPosition, 1.0);
    gl_Position  = ubo.proj * viewPos;
    fragViewDist = length(viewPos.xyz);
    fragViewX    = viewPos.x;
}
