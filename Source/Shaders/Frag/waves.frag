#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in float fragHeight;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(0, 0.2, 0.96, .05);
}

