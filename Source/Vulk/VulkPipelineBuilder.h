#pragma once

#include <vulkan/vulkan.h>

class VulkPipelineBuilder {
    Vulk &vk;
    std::vector<VkShaderModule> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkVertexInputBindingDescription bindingDescription = {};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
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
    void addVertexInputField(uint32_t binding, uint32_t location, uint32_t offset, VkFormat format);
public:
    VulkPipelineBuilder(Vulk &vk) : vk(vk) {}
    
    void addVertexShaderStage(char const *path) {
        addShaderStage(VK_SHADER_STAGE_VERTEX_BIT, path);
    }

    void addFragmentShaderStage(char const *path) {
        addShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, path);
    }

    // The binding says 'verts are in binding 0', and the stride says 'this is how far apart each vertex is'
    // then the field describe fields within the vertices: pos, normal, etc.
    void addVertexInputBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);
    void addVertexInputFieldVec3(uint32_t binding, uint32_t location, uint32_t fieldOffset);
    void addVertexInputFieldVec2(uint32_t binding, uint32_t location, uint32_t fieldOffset);

    void build(VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout &pipelineLayout, VkPipeline &graphicsPipeline);
};