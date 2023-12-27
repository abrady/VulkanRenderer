#pragma once

#include <vulkan/vulkan.h>

class VulkPipelineBuilder {
    Vulk &vk;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    VkVertexInputBindingDescription bindingDescription = {};
    std::array<VkVertexInputAttributeDescription, Vertex::NumBindingLocations> attributeDescriptions = {};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkPipelineViewportStateCreateInfo viewportState{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    std::vector<VkDynamicState> dynamicStates;
    VkPipelineDynamicStateCreateInfo dynamicState{};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

    void addShaderStage(VkShaderStageFlagBits stage, char const *path);
public:
    VulkPipelineBuilder(Vulk &vk, VkRenderPass renderPass, VkExtent2D swapChainExtent) : vk(vk) {}
    ~VulkPipelineBuilder();

    void addVertexShaderStage(char const *path) {
        addShaderStage(VK_SHADER_STAGE_VERTEX_BIT, path);
    }
    void addFragmentShaderStage(char const *path) {
        addShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, path);
    }

    void addVertexInputBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate);
    void addVertexInputAttributeDescription(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset);
    void addViewport(VkViewport viewport);
    void addScissor(VkRect2D scissor);
    void addRasterizerCreateInfo(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
    void addMultisamplingCreateInfo(VkSampleCountFlagBits sampleCount);
    void addColorBlendAttachmentState(VkColorComponentFlags colorWriteMask, VkBool32 blendEnable);
    void addColorBlendingCreateInfo(VkBool32 logicOpEnable, VkLogicOp logicOp);
    void addDepthStencilCreateInfo(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp);
    void addPipelineLayoutCreateInfo(VkDescriptorSetLayout descriptorSetLayout);
    void addDynamicStateCreateInfo(VkDynamicState dynamicState);
    void addDynamicStateCreateInfo(VkDynamicState *dynamicStates, uint32_t dynamicStateCount);
    void addPipelineCacheCreateInfo(VkPipelineCache pipelineCache);
    void addPipelineCacheCreateInfo(VkPipelineCache *pipelineCaches, uint32_t pipelineCacheCount);
    void addPipelineCacheCreateInfo(VkPipelineCache pipelineCache, VkPipelineCache *pipelineCaches, uint32_t pipelineCacheCount);
    void addPipelineCacheCreateInfo(VkPipelineCache *pipelineCaches, uint32_t pipelineCacheCount, VkPipelineCache pipelineCache);
    void addPipelineCacheCreateInfo(VkPipelineCache *pipelineCaches, uint32_t pipelineCacheCount, VkPipelineCache *pipelineCaches2, uint32_t pipelineCacheCount2);
    void addPipelineCacheCreateInfo(VkPipelineCache pipelineCache, VkPipelineCache *pipelineCaches, uint32_t pipelineCacheCount, VkPipelineCache *pipelineCaches2, uint32_t pipelineCacheCount2);
    void addPipelineCacheCreateInfo(VkPipelineCache *pipelineCaches, uint32_t pipelineCacheCount, VkPipelineCache pipelineCache, VkPipelineCache *pipelineCaches2,  

}