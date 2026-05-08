#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragColor;

layout(set = 1, binding = 0) uniform UBO {
    mat4 mvp;
} ubo;

void main() {
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
    fragColor = abs(inNormal);
}
