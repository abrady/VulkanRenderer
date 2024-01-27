#version 450

#include "common.glsl"

layout(binding = VulkShaderBinding_XformsUBO) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} xform;

layout(binding = VulkShaderBinding_MirrorPlaneUBO) uniform MirroredPlaneUBO {
    vec4 normal;
    vec4 point;
} mirrorPlane;

layout(location = LayoutLocation_Position) in vec3 inPosition;
layout(location = LayoutLocation_Normal) in vec3 inNormal;
layout(location = LayoutLocation_Tangent) in vec3 inTangent;
layout(location = LayoutLocation_TexCoord) in vec2 inTexCoord;

layout(location = LayoutLocation_Position) out vec3 fragPos;  
layout(location = LayoutLocation_TexCoord) out vec2 fragTexCoord;

// So reflecting a point across an arbitrary plain is really straightforward:
// Given: 
//     a plane (a point D on the plain and a normal N) and a point P
// you can reflect the point across that plane by moving it twice the distance in the direction of the plane:
//     P' = P - 2*N.(P-D)
void main() {
    vec4 worldPos = xform.model * vec4(inPosition, 1.0);
    vec4 mirrorNormal = xform.model * mirrorPlane.normal;
    vec4 mirrorPoint = xform.model * mirrorPlane.point;
    vec4 mirroredPos = worldPos - 2.0 * dot(worldPos - mirrorPoint, mirrorNormal) * mirrorNormal;
    gl_Position = xform.proj * xform.view * mirroredPos;
    fragTexCoord = inTexCoord;
    fragPos = vec3(xform.model * vec4(inPosition, 1.0));
}
