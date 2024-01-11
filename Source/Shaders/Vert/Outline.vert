#version 450

#include "common.glsl"

layout(binding = 0) uniform UniformBufferObject {
    mat4 world;
    mat4 view;
    mat4 proj;
} ubo;

layout(binding = VulkShaderBinding_ModelXform) uniform ModelXform {
    mat4 model;
} modelXform;

layout(location = LayoutLocation_Position) in vec3 inPosition;
layout(location = LayoutLocation_Normal) in vec3 inNormal;
layout(location = LayoutLocation_Tangent) in vec3 inTangent;
layout(location = LayoutLocation_TexCoord) in vec2 inTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.world * modelXform.model * vec4(inPosition, 1.0);
}