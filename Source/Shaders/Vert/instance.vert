#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 world;
    mat4 view;
    mat4 proj;
} ubo;

layout(std430, binding = 2) buffer ActorInstanceBufferObjects {
    mat4 model[];
} instance;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.world * instance.model[gl_InstanceIndex] * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}