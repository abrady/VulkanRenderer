#pragma once

#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <string>

#include "Vulk.h"
#include "VulkTextureView.h"
#include "VulkMesh.h"
#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkFrameUBOs.h"
#include "VulkBufferBuilder.h"
#include "VulkDescriptorSetBuilder.h"
#include "VulkPipelineBuilder.h"

struct VulkMeshResources
{
    Vulk &vk;
    VulkMesh mesh;
    VulkTextureView textureView;
    VulkTextureView normalView;
    uint32_t numIndices;
    VulkBuffer vertBuf, indexBuf;
    VulkFrameUBOs<VulkMaterial> materialUBOs;
    std::unique_ptr<VulkDescriptorSetInfo> dsInfo;

    VulkMeshResources(Vulk &vk, VulkMesh &&meshIn, std::filesystem::path const &texturePath, std::filesystem::path const &normalPath, VulkMaterial material)
        : vk(vk),
          mesh(std::move(meshIn)),
          textureView(vk, texturePath),
          normalView(vk, normalPath),
          numIndices((uint32_t)mesh.indices.size()),
          vertBuf(VulkBufferBuilder(vk)
                      .setSize(sizeof(mesh.vertices[0]) * mesh.vertices.size())
                      .setMem(mesh.vertices.data())
                      .setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
                      .setProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                      .build()),
          indexBuf(VulkBufferBuilder(vk)
                       .setSize(sizeof(mesh.indices[0]) * mesh.indices.size())
                       .setMem(mesh.indices.data())
                       .setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
                       .setProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                       .build()),
          materialUBOs(vk, material) {}

    void setDescriptorSets(std::unique_ptr<VulkDescriptorSetInfo> dsi)
    {
        this->dsInfo = std::move(dsi);
    }
};

// load resources used for rendering a set of things: shaders, meshes, textures, materials, etc.
// Note:
//  not thread safe,
//  not currently using reference counting so materials load twice
class VulkResources
{
public:
    Vulk &vk;

    struct XformsUBO
    {
        alignas(16) glm::mat4 world;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct ModelUBOs
    {
        VulkFrameUBOs<XformsUBO> xforms;
        VulkFrameUBOs<glm::vec3> eyePos;
        VulkFrameUBOs<VulkLight> lights;
        ModelUBOs(Vulk &vk) : xforms(vk), eyePos(vk), lights(vk) {}
    };
    ModelUBOs modelUBOs;

private:
    std::unordered_map<std::string, std::unique_ptr<VulkFrameUBOs<VulkMaterial>>> materials;
    std::unordered_map<std::string, std::unique_ptr<VulkDescriptorSetLayout>> descriptorSetLayouts;
    std::unordered_map<std::string, std::unique_ptr<VulkPipeline>> pipelines;
    std::unordered_map<std::string, std::unique_ptr<VulkMeshResources>> meshResources;
    std::unordered_map<std::string, VkShaderModule> vertShaders, fragShaders;
    VkSampler textureSampler;

    enum ShaderType
    {
        Vertex,
        Fragment
    };

    VkShaderModule createShaderModule(ShaderType type, std::string const &name);
    void loadMetadata();

    VulkResources &addMesh(std::string const &name, std::unique_ptr<VulkMeshResources> &&mesh);
    void loadDescriptorSetLayouts();
    void loadPipelines();

public:
    VulkResources(Vulk &vk)
        : vk(vk), modelUBOs(vk)
    {
        loadMetadata();
        textureSampler = vk.createTextureSampler();
        loadDescriptorSetLayouts();
        loadPipelines();
    }

    VulkResources &loadActor(std::string name); // load from the metadata for rendering
    VulkResources &loadVertexShader(std::string name);
    VulkResources &loadFragmentShader(std::string name);

    VkShaderModule getVertexShader(std::string const &name);
    VkShaderModule getFragmentShader(std::string const &name);
    VkSampler getTextureSampler();
    VulkMeshResources const *getMesh(std::string const &name);
    VkPipeline getPipeline(std::string const &name);
    VkPipelineLayout getPipelineLayout(std::string const &name);

    ~VulkResources();
};