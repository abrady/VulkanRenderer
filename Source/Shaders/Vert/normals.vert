#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 3) out vec3 normOut;
layout(location = 4) out uint instanceIDOut;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    normOut = inNormal;
    instanceIDOut = gl_InstanceIndex;
}