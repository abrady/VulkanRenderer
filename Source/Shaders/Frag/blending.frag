#version 450

#include "common.glsl"
layout(binding = VulkShaderBinding_TextureSampler) uniform sampler2D opacitySampler;
layout(binding = VulkShaderBinding_TextureSampler2) uniform sampler2D colorSampler;

layout(location = LayoutLocation_TexCoord) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(opacitySampler, fragTexCoord);
    if (all(lessThan(vec3(color), vec3(.1)))) {
        discard;
    }
    outColor = texture(colorSampler, fragTexCoord);
}