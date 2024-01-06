#version 450

#include "common.glsl"

layout(binding = VulkShaderBinding_TextureSampler) uniform sampler2D beachSampler;
layout(binding = VulkShaderBinding_TextureSampler2) uniform sampler2D grassSampler;
layout(binding = VulkShaderBinding_TextureSampler3) uniform sampler2D snowSampler;

layout(binding = VulkShaderBinding_EyePos) uniform EyePos {
    vec3 eyePos;
} eyePosUBO;

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;  // Fragment position in world space
layout(location = 3) in vec3 fragNormal;   // Normal vector in world space
layout(location = 4) in float fragHeight;

layout(std430, binding = VulkShaderBinding_Lights) buffer LightBuf {
    Light light;
} lightBuf;

layout(std430, binding = VulkShaderBinding_Materials) buffer MaterialBuf {
    Material material;
} materialBuf;

layout(location = 0) out vec4 outColor;

vec4 getTerrainDiffuse() {
    float beachStart = -10.0;
    float beachEnd = 0.0;
    float snowStart = 12.0;
    float snowEnd = 20.0;

    // Calculate the blend factors for beach-grass and grass-snow transitions.
    float beachGrassBlend = smoothstep(beachStart, beachEnd, fragHeight);
    float grassSnowBlend = smoothstep(snowStart, snowEnd, fragHeight);

    // Blend textures
    vec4 beachColor = texture(beachSampler, fragTexCoord);
    vec4 grassColor = texture(grassSampler, fragTexCoord);
    vec4 snowColor = texture(snowSampler, fragTexCoord);

    // Blend beach and grass based on height, then blend that result with snow.
    // this works because if we're above beachEnd, we'll havve all grass color
    // then if we're above snowStart, we'll have all snow color
    vec4 beachGrassColor = mix(beachColor, grassColor, beachGrassBlend);
    vec4 finalColor = mix(beachGrassColor, snowColor, grassSnowBlend);
    return 2*finalColor;
}

void main() {
    vec4 terrainDiffuse = getTerrainDiffuse();
    outColor = basicLighting(lightBuf.light, materialBuf.material, terrainDiffuse, eyePosUBO.eyePos, fragNormal, fragPos);
}
