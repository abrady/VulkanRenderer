#version 450

#include "common.glsl"

layout(binding = VulkShaderBinding_XformsUBO) uniform UniformBufferObject {
    mat4 world;
    mat4 view;
    mat4 proj;
} ubo;

layout(std430, binding = VulkShaderBinding_Actors) buffer ActorInstanceBufferObjects {
    mat4 model[];
} instance;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out float fragHeight;

void main() {
    mat4 worldMat = ubo.world * instance.model[gl_InstanceIndex];
    vec4 worldPos = worldMat * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    fragTexCoord = inTexCoord;
    fragPos = vec3(worldPos);
    fragNormal = mat3(worldMat) * inNormal;
    fragHeight = inPosition.y;
}