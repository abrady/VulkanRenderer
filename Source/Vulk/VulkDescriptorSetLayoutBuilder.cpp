#include "VulkDescriptorSetLayoutBuilder.h"
#include "VulkUtil.h"
#include "Vulk.h"

VulkDescriptorSetLayoutBuilder &VulkDescriptorSetLayoutBuilder::addUniformBuffer(uint32_t binding, VkShaderStageFlags stageFlags)
{
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = stageFlags;
    ASSERT_KEY_NOT_SET(layoutBindingsMap, binding);
    layoutBindingsMap[binding] = layoutBinding;
    return *this;
}

VulkDescriptorSetLayoutBuilder &VulkDescriptorSetLayoutBuilder::addSampler(uint32_t binding, VkShaderStageFlags stageFlags)
{
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = stageFlags;
    ASSERT_KEY_NOT_SET(layoutBindingsMap, binding);
    layoutBindingsMap[binding] = layoutBinding;
    return *this;
}

VulkDescriptorSetLayoutBuilder &VulkDescriptorSetLayoutBuilder::addStorageBuffer(uint32_t binding, VkShaderStageFlags stageFlags)
{
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = stageFlags;
    ASSERT_KEY_NOT_SET(layoutBindingsMap, binding);
    layoutBindingsMap[binding] = layoutBinding;
    return *this;
}

VkDescriptorSetLayout VulkDescriptorSetLayoutBuilder::build()
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    layoutBindings.reserve(layoutBindingsMap.size());
    for (auto &pair : layoutBindingsMap)
    {
        layoutBindings.push_back(pair.second);
    }
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutCreateInfo.pBindings = layoutBindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    VK_CALL(vkCreateDescriptorSetLayout(vk.device, &layoutCreateInfo, nullptr, &descriptorSetLayout));
    return descriptorSetLayout;
}
