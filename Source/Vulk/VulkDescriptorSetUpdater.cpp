#include "VulkDescriptorSetUpdater.h"

VulkDescriptorSetUpdater &VulkDescriptorSetUpdater::addUniformBuffer(VkBuffer buf, VkDeviceSize range, uint32_t binding)
{
    auto uniformBufferInfo = std::make_unique<VkDescriptorBufferInfo>();
    uniformBufferInfo->buffer = buf;
    uniformBufferInfo->offset = 0;
    uniformBufferInfo->range = range;

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = descriptorSet;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.pBufferInfo = uniformBufferInfo.get();
    descriptorWrites.push_back(writeDescriptorSet);
    bufferInfos.push_back(std::move(uniformBufferInfo));
    return *this;
}

VulkDescriptorSetUpdater &VulkDescriptorSetUpdater::addImageSampler(VkImageView textureImageView, VkSampler textureSampler, uint32_t binding)
{
    auto imageInfo = std::make_unique<VkDescriptorImageInfo>();
    imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo->imageView = textureImageView;
    imageInfo->sampler = textureSampler;

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = descriptorSet;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.pImageInfo = imageInfo.get();

    descriptorWrites.push_back(writeDescriptorSet);
    imageInfos.push_back(std::move(imageInfo));
    return *this;
}

VulkDescriptorSetUpdater &VulkDescriptorSetUpdater::addStorageBuffer(VkBuffer buf, VkDeviceSize range, uint32_t binding)
{
    auto storageInfo = std::make_unique<VkDescriptorBufferInfo>();
    storageInfo->buffer = buf;
    storageInfo->offset = 0;
    storageInfo->range = range;

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = descriptorSet;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.pBufferInfo = storageInfo.get();

    descriptorWrites.push_back(writeDescriptorSet);
    bufferInfos.push_back(std::move(storageInfo));
    return *this;
}

void VulkDescriptorSetUpdater::update(VkDevice device)
{
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}