#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class VulkDescriptorPoolBuilder
{
private:
    Vulk &vk;
    std::unordered_map<VkDescriptorType, VkDescriptorPoolSize> poolSizes;

    VulkDescriptorPoolBuilder &addPoolSizeCount(VkDescriptorType type, uint32_t count)
    {
        uint32_t existingCount = 0;
        if (poolSizes.find(type) != poolSizes.end())
        {
            existingCount = poolSizes[type].descriptorCount;
        }
        VkDescriptorPoolSize poolSize{};
        poolSize.type = type;
        poolSize.descriptorCount = existingCount + count;
        poolSizes[type] = poolSize;
        return *this;
    }

public:
    VulkDescriptorPoolBuilder(Vulk &vk) : vk(vk) {}
    VulkDescriptorPoolBuilder &addUniformBufferCount(uint32_t count)
    {
        return addPoolSizeCount(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);
    }

    VulkDescriptorPoolBuilder &addCombinedImageSamplerCount(uint32_t count)
    {
        return addPoolSizeCount(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count);
    }

    VulkDescriptorPoolBuilder &addSampledImageCount(uint32_t count)
    {
        return addPoolSizeCount(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, count);
    }

    VulkDescriptorPoolBuilder &addStorageBufferCount(uint32_t count)
    {
        return addPoolSizeCount(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count);
    }

    VkDescriptorPool build(uint32_t maxSets)
    {
        std::vector<VkDescriptorPoolSize> poolSizesVector;
        for (auto &kv : poolSizes)
        {
            poolSizesVector.push_back(kv.second);
        }
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizesVector.size());
        poolInfo.pPoolSizes = poolSizesVector.data();
        poolInfo.maxSets = maxSets;

        VkDescriptorPool descriptorPool;
        VK_CALL(vkCreateDescriptorPool(vk.device, &poolInfo, nullptr, &descriptorPool));

        return descriptorPool;
    }
};