#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class VulkDescriptorPoolBuilder {
private:
    std::vector<VkDescriptorPoolSize> poolSizes;

    VulkDescriptorPoolBuilder& addPoolSizeCount(VkDescriptorType type, uint32_t count) {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = type;
        poolSize.descriptorCount = count;
        poolSizes.push_back(poolSize);
        return *this;
    }
public:
    VulkDescriptorPoolBuilder& addUniformBufferCount(uint32_t count) {
        return addPoolSizeCount(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);
    }

    VulkDescriptorPoolBuilder& addCombinedImageSamplerCount(uint32_t count) {
        return addPoolSizeCount(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count);
    }

    VulkDescriptorPoolBuilder& addSampledImageCount(uint32_t count) {
        return addPoolSizeCount(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, count);
    }

    VulkDescriptorPoolBuilder& addStorageBufferCount(uint32_t count) {
        return addPoolSizeCount(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);
    }

    VkDescriptorPool build(VkDevice device, uint32_t maxSets) {
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = maxSets;

        VkDescriptorPool descriptorPool;
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

        return descriptorPool;
    }
};