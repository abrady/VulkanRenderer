#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <unordered_map>

class Vulk;

class VulkDescriptorSetLayoutBuilder
{
    Vulk &vk;

public:
    VulkDescriptorSetLayoutBuilder(Vulk &vk) : vk(vk) {}
    VulkDescriptorSetLayoutBuilder &addUniformBuffer(uint32_t binding, VkShaderStageFlags stageFlags);
    VulkDescriptorSetLayoutBuilder &addSampler(uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);
    VulkDescriptorSetLayoutBuilder &addStorageBuffer(uint32_t binding, VkShaderStageFlags stageFlags);

    // and finally, build the layout
    VkDescriptorSetLayout build();

private:
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> layoutBindingsMap;
};
