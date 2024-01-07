const float PI = 3.1415926535897932384626433832795;

const int VulkShaderBinding_XformsUBO = 0;
const int VulkShaderBinding_TextureSampler = 1;
const int VulkShaderBinding_Actors = 2;
const int VulkShaderBinding_Lights = 3;
const int VulkShaderBinding_Materials = 4;
const int VulkShaderBinding_EyePos = 5;
const int VulkShaderBinding_TextureSampler2 = 6;
const int VulkShaderBinding_TextureSampler3 = 7;
const int VulkShaderBinding_WavesXform = 8;

// note that the order matters here: it allows this to be packed into 3 vec4s
struct Light {
    vec3 pos;           // point light only
    float falloffStart; // point/spot light only
    vec3 color;         // color of light
    float falloffEnd;   // point/spot light only    
    vec3 direction;     // directional/spot light only
    float spotPower;    // spotlight only
};

struct Material {
    vec4 diffuse;
    vec3 fresnelR0;
    float roughness;
};

/**
* @brief Calculates the lighting for a single light source using the Phong lighting model.
* @param diffuseIn: an additional diffuse color to be added to the material's diffuse color. typically from a texture.
*/
vec4 basicLighting(Light light, Material material, vec4 diffuseIn, vec3 eyePos, vec3 fragNormal, vec3 fragPos) {
    fragNormal = normalize(fragNormal);
    vec4 lightColor = vec4(light.color, 1.0);
    vec3 lightDir = normalize(light.pos - fragPos);
    float lambert = max(dot(fragNormal, lightDir), 0.0);
    vec3 normEyeVec = normalize(eyePos - fragPos);

    vec4 matDiffuse = material.diffuse * diffuseIn;

    // A_l⊗m_d
    vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0) * matDiffuse;

    // B*max(N . L, 0)⊗m_d
    vec4 diffuse = lambert * lightColor * matDiffuse;

    // max(L.n,0)B⊗[F_0 + (1 - F_0)(1 - N.v)^5][(m + 8)/8(n.h)^m]
    vec4 R0 = vec4(material.fresnelR0, 1.0);
    vec3 h = normalize(normEyeVec + lightDir);
    vec4 fresnel = R0 + (vec4(1.0) - R0) * pow(1.0 - dot(fragNormal,normEyeVec), 5.0);
    float m = material.roughness * 8.0;
    float nDotH = max(dot(fragNormal, h), 0);
    float D = (m + 2.0) / (2.0 * PI) * pow(nDotH, m);
    vec4 microfacet = vec4(D);
    vec4 specular = lightColor * lambert * fresnel * microfacet;

    // combine
    vec4 acc = ambient;
    acc += diffuse;
    acc += specular;
    return acc;
    // return (ambient + diffuse + specular);
}

const int LayoutLocation_Position = 0;
const int LayoutLocation_TexCoord = 1;
const int LayoutLocation_Pos = 2;
const int LayoutLocation_Normal = 3; 
const int LayoutLocation_Height = 4;

#define LAYOUT_VULKVERTEX_IN  \
layout(location = 0) in vec3 inPosition; \
layout(location = 1) in vec3 inNormal; \
layout(location = 2) in vec3 inTangent; \
layout(location = 3) in vec2 inTexCoord
