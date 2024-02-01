#pragma once

#include "VulkUtil.h"
#include "Common/ClassNonCopyableNonMovable.h"
class Vulk;

class VulkDescriptorSetLayout : public ClassNonCopyableNonMovable
{
    Vulk &vk;

public:
    VkDescriptorSetLayout layout;
    VulkDescriptorSetLayout(Vulk &vk, VkDescriptorSetLayout layout);
    ~VulkDescriptorSetLayout();
};

class VulkDescriptorSetLayoutBuilder
{
    Vulk &vk;

public:
    VulkDescriptorSetLayoutBuilder(Vulk &vk) : vk(vk) {}
    void foo() {}
    VulkDescriptorSetLayoutBuilder &addUniformBuffer(VkShaderStageFlags stageFlags, VulkShaderUBOBindings binding);
    VulkDescriptorSetLayoutBuilder &addImageSampler(VkShaderStageFlags stageFlags, VulkShaderTextureBindings binding);
    VulkDescriptorSetLayoutBuilder &addStorageBuffer(VkShaderStageFlags stageFlags, VulkShaderSSBOBindings binding);

    // and finally, build the layout
    std::unique_ptr<VulkDescriptorSetLayout> build();

private:
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> layoutBindingsMap;
};
