#version 450

#include "common.glsl"

layout(binding = 0) uniform UniformBufferObject {
    mat4 world;
    mat4 viewproj;
} ubo;


LAYOUT_VULKVERTEX_IN;
layout(location = LayoutLocation_TexCoord) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.viewproj * ubo.world * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}