#version 450

#include "common.glsl"

layout(binding = 0) uniform GlobalXformBuf {
    GlobalXform xform;
} ubo;


LAYOUT_VULKVERTEX_IN;
layout(location = LayoutLocation_TexCoord) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.xform.proj * ubo.xform.view * ubo.xform.world * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}