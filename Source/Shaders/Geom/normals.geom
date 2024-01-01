#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 world;
    mat4 view;
    mat4 proj;
} ubo;

layout(std430, binding = 2) buffer ActorInstanceBufferObjects {
    mat4 model[];
} instance;

layout(location = 3) in vec3 norms[];   // Normal vector in world space
layout(location = 4) in uint instanceIds[]; // Instance ID

layout (points) in;
layout (line_strip, max_vertices = 2) out;
layout(location = 3) out vec3 normOut;

void main() {
    mat4 world = ubo.world * instance.model[instanceIds[0]];
    mat4 projView = ubo.proj * ubo.view;
    vec3 worldNorm = vec3(world * vec4(norms[0], 0.0));
    vec4 worldPos = world * gl_in[0].gl_Position;
    
    // Output the original point
    normOut = worldNorm;
    gl_Position = projView * worldPos;
    EmitVertex();

    // Output a second point offset by the normal
    normOut = worldNorm;
    vec4 offsetPos = worldPos + 10.f * vec4(worldNorm, 0.0);
    gl_Position = projView * offsetPos;
    EmitVertex();

    EndPrimitive();
}
