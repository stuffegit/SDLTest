#version 450

layout(location = 0) in float fragViewDist;
layout(location = 1) in float fragViewX;
layout(location = 2) in vec3  fragNormal;
layout(location = 3) in vec3  fragViewPos;
layout(location = 4) in vec2  fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D texSampler;

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
    vec4  lightDir;
    vec4  lightColor;
    vec4  ambientColor;
    float shininess;
    float specStrength;
    float _lpad0;
    float _lpad1;
} u;

void main() {
    vec4 texSample = texture(texSampler, vec2(fragUV.x, 1.0 - fragUV.y));
    vec3 baseColor = u.color.rgb * texSample.rgb;

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(u.lightDir.xyz);
    vec3 V = normalize(-fragViewPos);
    vec3 R = reflect(-L, N);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(R, V), 0.0), u.shininess) * u.specStrength;

    vec3 ambient  = u.ambientColor.rgb * baseColor;
    vec3 diffuse  = u.lightColor.rgb * baseColor * diff;
    vec3 specular = u.lightColor.rgb * spec;

    vec3 litColor = ambient + diffuse + specular;

    float t_depth = clamp((fragViewDist - u.fogStart) / (u.fogEnd - u.fogStart), 0.0, 1.0);
    float t_side  = clamp((abs(fragViewX) - u.sideStart) / (u.sideEnd - u.sideStart), 0.0, 1.0);
    float t = 1.0 - (1.0 - t_depth) * (1.0 - t_side);
    outColor = vec4(litColor * (1.0 - t * u.fogStrength), u.color.a * texSample.a);
}
