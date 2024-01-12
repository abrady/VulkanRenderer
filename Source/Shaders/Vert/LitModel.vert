#version 450

#include "common.glsl"

layout(binding = VulkShaderBinding_XformsUBO) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} xform;

layout(location = LayoutLocation_Position) in vec3 inPosition;
layout(location = LayoutLocation_Normal) in vec3 inNormal;
layout(location = LayoutLocation_Tangent) in vec3 inTangent;
layout(location = LayoutLocation_TexCoord) in vec2 inTexCoord;

layout(location = LayoutLocation_Position) out vec3 fragPos;  
layout(location = LayoutLocation_TexCoord) out vec2 fragTexCoord;

void main() {
    gl_Position = xform.proj * xform.view * xform.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragPos = vec3(xform.model * vec4(inPosition, 1.0));
}