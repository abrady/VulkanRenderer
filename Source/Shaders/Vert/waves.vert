#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 world;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out float fragHeight;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.world * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragHeight = inPosition.y;
}