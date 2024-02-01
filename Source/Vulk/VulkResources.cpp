#include "VulkResources.h"

#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

#include "VulkMesh.h"
#include "VulkPipelineBuilder.h"
#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkDescriptorSetBuilder.h"
#include "private/VulkResourceMetadata.h"

void VulkResources::loadMetadata()
{
    static std::once_flag flag;
    std::call_once(flag, [&]()
                   { findAndProcessMetadata(std::filesystem::current_path() / "Assets"); });
}

VkShaderModule VulkResources::createShaderModule(ShaderType type, std::string const &name)
{
    std::filesystem::path subdir;
    switch (type)
    {
    case Vertex:
        subdir = "Vert";
        break;
    case Fragment:
        subdir = "Frag";
        break;
    };

    std::filesystem::path path = std::filesystem::current_path() / "Source" / "Shaders" / subdir / (name + ".spv");
    auto shaderCode = readFileIntoMem(path.string());
    VkShaderModule shaderModule = vk.createShaderModule(shaderCode);
    return shaderModule;
}

VulkResources &VulkResources::loadActor(std::string name)
{
    ASSERT_KEY_SET(metadata.models, name);
    if (meshResources.find(name) != meshResources.end())
    {
        return *this;
    }
    ModelDef &modelDef = metadata.models[name];
    std::filesystem::path modelPath = modelDef.directoryPath / modelDef.mesh;
    materials[name] = std::make_unique<VulkFrameUBOs<VulkMaterial>>(vk, modelDef.material);
    auto mr = std::make_unique<VulkMeshResources>(vk, VulkMesh::loadFromPath(modelPath), modelDef.directoryPath / modelDef.texture, modelDef.directoryPath / modelDef.normalMap, modelDef.material);
    mr->setDescriptorSets(VulkDescriptorSetBuilder(vk)
                              .addUniformBuffers(modelUBOs.xforms.getUBOs(), VK_SHADER_STAGE_VERTEX_BIT, VulkShaderUBOBinding_Xforms)
                              .addUniformBuffers(modelUBOs.eyePos.getUBOs(), VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_EyePos)
                              .addUniformBuffers(modelUBOs.lights.getUBOs(), VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_Lights)
                              .addUniformBuffers(materials.at(name)->getUBOs(), VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_MaterialUBO)
                              .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_TextureSampler, mr->textureView.textureImageView, getTextureSampler())
                              .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_NormalSampler, mr->normalView.textureImageView, getTextureSampler())
                              .build());
    // loads and owns the mesh-specific resources (texture, normal map, descriptor set)
    meshResources[name] = std::move(mr);

    return *this;
}

VulkResources &VulkResources::loadVertexShader(std::string name)
{
    ASSERT_KEY_NOT_SET(vertShaders, name);
    VkShaderModule shaderModule = createShaderModule(Vertex, name);
    vertShaders[name] = shaderModule;
    return *this;
}

VulkResources &VulkResources::loadFragmentShader(std::string name)
{
    ASSERT_KEY_NOT_SET(fragShaders, name);
    VkShaderModule shaderModule = createShaderModule(Fragment, name);
    fragShaders[name] = shaderModule;
    return *this;
}

VulkResources &VulkResources::addMesh(std::string const &name, std::unique_ptr<VulkMeshResources> &&mesh)
{
    ASSERT_KEY_NOT_SET(meshResources, name);
    meshResources[name] = std::move(mesh);
    return *this;
}

void VulkResources::loadDescriptorSetLayouts()
{
    descriptorSetLayouts["LitModel"] = VulkDescriptorSetLayoutBuilder(vk)
                                           .addUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, VulkShaderUBOBinding_Xforms)
                                           .addUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_EyePos)
                                           .addUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_Lights)
                                           .addUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_MaterialUBO)
                                           .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_TextureSampler)
                                           .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_NormalSampler)
                                           .build();
}

void VulkResources::loadPipelines()
{
    pipelines["LitModel"] = VulkPipelineBuilder(vk)
                                .addVertexShaderStage(getVertexShader("LitModel"))
                                .addFragmentShaderStage(getFragmentShader("LitModel"))
                                .addVulkVertexInput(0)
                                .build(descriptorSetLayouts.at("LitModel")->layout);
}

VkShaderModule VulkResources::getVertexShader(std::string const &name)
{
    if (vertShaders.find(name) == vertShaders.end())
    {
        loadVertexShader(name);
    }
    ASSERT_KEY_SET(vertShaders, name);
    return vertShaders[name];
}

VkShaderModule VulkResources::getFragmentShader(std::string const &name)
{
    if (fragShaders.find(name) == fragShaders.end())
    {
        loadFragmentShader(name);
    }
    ASSERT_KEY_SET(fragShaders, name);
    return fragShaders[name];
}

VkSampler VulkResources::getTextureSampler()
{
    return textureSampler;
}

VulkMeshResources const *VulkResources::getMesh(std::string const &name)
{
    ASSERT_KEY_SET(meshResources, name);
    return meshResources[name].get();
}

VkPipeline VulkResources::getPipeline(std::string const &name)
{
    ASSERT_KEY_SET(pipelines, name);
    VulkPipeline *p = pipelines.at(name).get();
    assert(p->pipeline != VK_NULL_HANDLE);
    return p->pipeline;
}

VkPipelineLayout VulkResources::getPipelineLayout(std::string const &name)
{
    ASSERT_KEY_SET(pipelines, name);
    VulkPipeline *p = pipelines.at(name).get();
    assert(p->pipelineLayout != VK_NULL_HANDLE);
    return p->pipelineLayout;
}

VulkResources::~VulkResources()
{
    for (auto &pair : vertShaders)
    {
        vkDestroyShaderModule(vk.device, pair.second, nullptr);
    }
    for (auto &pair : fragShaders)
    {
        vkDestroyShaderModule(vk.device, pair.second, nullptr);
    }
    vkDestroySampler(vk.device, textureSampler, nullptr);
}
