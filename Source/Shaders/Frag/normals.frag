#version 450

layout(location = 3) in vec3 norm;   // Normal vector in world space
layout(location = 0) out vec4 outColor;
void main()
{
    vec3 normalColor = normalize(norm);  // Normalize the normal
    normalColor = normalColor * 0.5 + 0.5; // Scale and bias to fit 0 to 1
    outColor = vec4(normalColor, 1.0);
}
