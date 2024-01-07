#version 450

#include "common.glsl"
layout(binding = 1) uniform sampler2D texSampler;

layout(location = LayoutLocation_TexCoord) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord);
}