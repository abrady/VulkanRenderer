#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class Vulk;

class VulkDescriptorSetLayoutBuilder {
public:
    void addUniformBuffer(uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT);
    void addSampler(uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);
    void addStorageBuffer(uint32_t binding, VkShaderStageFlags stageFlags);

    // and finally, build the layout
    void build(Vulk &vk, VkDescriptorSetLayout &descriptorSetLayout);
private:
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    VkDevice device; // You need to provide the device handle
};
