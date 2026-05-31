#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out float fragViewDist;
layout(location = 1) out float fragViewX;
layout(location = 2) out vec3  fragNormal;
layout(location = 3) out vec3  fragViewPos;
layout(location = 4) out vec2  fragUV;

layout(set = 1, binding = 0) uniform UBO {
    mat4 modelView;
    mat4 proj;
    mat4 normalMatrix;  // transpose(inverse(mat3(modelView)))
} ubo;

void main() {
    vec4 viewPos = ubo.modelView * vec4(inPosition, 1.0);
    gl_Position  = ubo.proj * viewPos;
    fragViewDist = length(viewPos.xyz);
    fragViewX    = viewPos.x;
    fragNormal   = normalize(mat3(ubo.normalMatrix) * inNormal);
    fragViewPos  = viewPos.xyz;
    fragUV       = inUV;
}
