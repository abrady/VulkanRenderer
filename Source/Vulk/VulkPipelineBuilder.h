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


    VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VulkPipelineBuilder& addShaderStage(VkShaderStageFlagBits stage, char const *path);
    VulkPipelineBuilder& addVertexInputField(uint32_t binding, uint32_t location, uint32_t offset, VkFormat format);
public:
    VulkPipelineBuilder(Vulk &vk);
    
    VulkPipelineBuilder& addVertexShaderStage(char const *path) {
        return addShaderStage(VK_SHADER_STAGE_VERTEX_BIT, path);
    }

    VulkPipelineBuilder& addFragmentShaderStage(char const *path) {
        return addShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, path);
    }

    VulkPipelineBuilder& addGeometryShaderStage(char const *path) {
        return addShaderStage(VK_SHADER_STAGE_GEOMETRY_BIT, path);
    }

    VulkPipelineBuilder& setPrimitiveTopology(VkPrimitiveTopology topology);
    VulkPipelineBuilder& setLineWidth(float lineWidth);
    VulkPipelineBuilder& setCullMode(VkCullModeFlags cullMode);
    VulkPipelineBuilder& setDepthTestEnabled(bool enabled);
    VulkPipelineBuilder& setDepthWriteEnabled(bool enabled);

    // The binding says 'verts are in binding 0', and the stride says 'this is how far apart each vertex is'
    // then the field describe fields within the vertices: pos, normal, etc.
    VulkPipelineBuilder& addVertexInputBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);
    VulkPipelineBuilder& addVertexInputFieldVec3(uint32_t binding, uint32_t location, uint32_t fieldOffset);
    VulkPipelineBuilder& addVertexInputFieldVec2(uint32_t binding, uint32_t location, uint32_t fieldOffset);

    void build(VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout &pipelineLayout, VkPipeline &graphicsPipeline);
};