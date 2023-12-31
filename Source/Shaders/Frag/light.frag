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

// note that the order matters here: it allows this to be packed into 3 vec4s
struct Light {
    vec3 pos;           // point light only
    vec4 color;         // color of light
    float falloffStart; // point/spot light only
    vec3 direction;     // directional/spot light only
    float falloffEnd;   // point/spot light only    
    float spotPower;    // spotlight only
};

layout(std430, binding = 3) buffer LightBuf {
    Light light;
} lightBuf;

layout(std430, binding = 4) buffer MaterialBuf {
    Material material;
} materialBuf;

layout(location = 0) out vec4 outColor;

float calcAttenuation(float dist, float falloffStart, float falloffEnd) {
    if (falloffEnd <= falloffStart) {
        return 1.0;
    }
    return clamp(1.0 - (dist - falloffStart) / (falloffEnd - falloffStart), 0.0, 1.0);
}

void main() {
    Light light = lightBuf.light;
    Material material = materialBuf.material;
    vec3 eyePos = eyePosUBO.eyePos;

    vec3 fragNormal = normalize(fragNormal);
    vec3 lightDir = normalize(light.pos - fragPos);
    float lambert = max(dot(fragNormal, lightDir), 0.0);
    vec3 v = normalize(eyePos - fragPos);

    // attenuate the lambert
    // float dist = length(light.pos - fragPos);
    // lambert *= calcAttenuation(dist, light.falloffStart, light.falloffEnd);

    // c_a + c_d + c_s = A_l⊗m_d + B⊗max(N . L, 0){m_d* + [F_0 + (1 - F_0)(1 - N.v)^5](m + 8)/8(n.h)^m}

    // A_l⊗m_d
    vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0) * material.diffuse;

    // B*max(N . L, 0)⊗m_d
    vec4 diffuse = lambert * light.color * material.diffuse;

    // max(L.n,0)B⊗[F_0 + (1 - F_0)(1 - N.v)^5][(m + 8)/8(n.h)^m]
    vec4 R0 = vec4(material.fresnelR0, 1.0);
    vec3 h = normalize(v + lightDir);
    vec4 fresnel = R0 + (vec4(1.0) - R0) * pow(1.0 - dot(fragNormal,v), 5.0);

    int m = int(material.roughness * 8);
    float nDotH = max(dot(fragNormal, h), 0);
    float powNDotH = pow(nDotH, m);
    vec4 microfacet = vec4((m + 8) / 8 * powNDotH);
    vec4 specular = lambert*(fresnel + microfacet);

    // combine
    outColor = (ambient + diffuse + specular);
}