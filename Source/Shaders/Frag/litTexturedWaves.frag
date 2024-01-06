#version 450

#include "common.glsl"

layout(binding = VulkShaderBinding_TextureSampler) uniform sampler2D texSampler;
layout(binding = VulkShaderBinding_EyePos) uniform EyePos {
    vec3 eyePos;
} eyePosUBO;

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;  // Fragment position in world space
layout(location = 3) in vec3 fragNormal;   // Normal vector in world space

layout(std430, binding = VulkShaderBinding_Lights) buffer LightBuf {
    Light light;
} lightBuf;

layout(std430, binding = VulkShaderBinding_Materials) buffer MaterialBuf {
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
    vec4 wavesDiffuse = texture(texSampler, fragTexCoord);
    outColor = basicLighting(lightBuf.light, materialBuf.material, wavesDiffuse, eyePosUBO.eyePos, fragNormal, fragPos);
}