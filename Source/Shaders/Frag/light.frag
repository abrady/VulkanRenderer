#version 450

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 5) uniform EyePos {
    vec3 eyePos;
} eyePosUBO;

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;  // Fragment position in world space
layout(location = 3) in vec3 fragNormal;   // Normal vector in world space

struct Material {
    vec4 diffuse;
    vec3 fresnelR0;
    float roughness;
};

struct Light {
    vec3 pos;
    vec4 color;
};

layout(std430, binding = 3) buffer LightBuf {
    Light light;
} lightBuf;

layout(std430, binding = 4) buffer MaterialBuf {
    Material material;
} materialBuf;

layout(location = 0) out vec4 outColor;

void main() {
    Light light = lightBuf.light;
    Material material = materialBuf.material;
    vec3 eyePos = eyePosUBO.eyePos;

    vec3 fragNormal = normalize(fragNormal);
    vec3 lightDir = normalize(light.pos - fragPos);
    float lambert = max(dot(fragNormal, lightDir), 0.0);
    vec3 v = normalize(eyePos - fragPos);

    // c_a + c_d + c_s = A_l⊗m_d + B⊗max(N . L, 0){m_d* + [F_0 + (1 - F_0)(1 - N.v)^5](m + 8)/8(n.h)^m}

    // A_l⊗m_d
    vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0) * material.diffuse;

    // B*max(N . L, 0)⊗m_d
    vec4 diffuse = lambert * light.color * material.diffuse;

    // max(L.n,0)B⊗[F_0 + (1 - F_0)(1 - N.v)^5][(m + 8)/8(n.h)^m]
    float m = material.roughness * 8;
    vec4 R0 = vec4(material.fresnelR0, 1.0);
    vec3 h = normalize(v + lightDir);
    vec4 fresnel = R0 + (vec4(1.0) - R0) * pow(1.0 - dot(fragNormal,v), 5.0);
    vec4 microfacet = vec4((m + 8) / 8 * pow(dot(fragNormal, h), m));
    vec4 specular = lambert*(fresnel + microfacet);

    // combine
    outColor = (ambient + diffuse + specular);
}