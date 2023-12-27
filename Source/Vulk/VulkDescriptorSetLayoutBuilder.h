#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class Vulk;

class VulkDescriptorSetLayoutBuilder {
public:
    VulkDescriptorSetLayoutBuilder& addUniformBuffer(uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT);
    VulkDescriptorSetLayoutBuilder& addSampler(uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);
    VulkDescriptorSetLayoutBuilder& addStorageBuffer(uint32_t binding, VkShaderStageFlags stageFlags);

    // and finally, build the layout
    VkDescriptorSetLayout build(Vulk &vk);
private:
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
};
