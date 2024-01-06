#pragma once 

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include "VulkStorageBuffer.h"


template<typename T>
class VulkStorageBuffer;

class VulkDescriptorSetUpdater {
private:
    std::vector<std::unique_ptr<VkDescriptorBufferInfo>> bufferInfos;
    std::vector<std::unique_ptr<VkDescriptorImageInfo>> imageInfos;
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    VkDescriptorSet descriptorSet;
public:
    VulkDescriptorSetUpdater(VkDescriptorSet descriptorSet) : descriptorSet(descriptorSet) {}

    VulkDescriptorSetUpdater& addUniformBuffer(VkBuffer buf, VkDeviceSize range, uint32_t binding);
    VulkDescriptorSetUpdater& addImageSampler(VkImageView textureImageView, VkSampler textureSampler, uint32_t binding);
    VulkDescriptorSetUpdater& addStorageBuffer(VkBuffer buf, VkDeviceSize range, uint32_t binding);
    template<typename T>
    VulkDescriptorSetUpdater& addVulkStorageBuffer(VulkStorageBuffer<T>& storageBuffer, uint32_t binding) {
        return addStorageBuffer(storageBuffer.buf, storageBuffer.getSize(), binding);
    }
    void update(VkDevice device);
};
