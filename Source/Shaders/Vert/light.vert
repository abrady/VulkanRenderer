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
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 fragNormal;

void main() {
    mat4 worldMat = ubo.world * instance.model[gl_InstanceIndex];
    gl_Position = ubo.proj * ubo.view * worldMat * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragPos = worldMat * vec4(inPosition, 1.0);

    // if we do any scaling or shearing, we need to use the inverse transpose of the world matrix
    // but since I'm not doing any of that, we can just use the world matrix
    //fragNormal = mat3(transpose(inverse(worldMat))) * inNormal;
    fragNormal = mat3(worldMat) * inNormal;
}