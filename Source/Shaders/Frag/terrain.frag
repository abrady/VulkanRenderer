#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in float fragHeight;

layout(location = 0) out vec4 outColor;

void main() {
    if (fragHeight < -10) {
        outColor = vec4(1.0, 0.96, 0.62, 1.0);
        return;
    } else if (fragHeight < 5) {
        outColor = vec4(0.48, 0.77, 0.46, 1.0);
        return;
    } else if (fragHeight < 12) {
        outColor = vec4(0.1, 0.48, 0.19, 1.0);
        return;
    } else if (fragHeight < 20) {
        outColor = vec4(0.45, 0.39, 0.34, 1.0);
        return;
    } else {
        outColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }
}

