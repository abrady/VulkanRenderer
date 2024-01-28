#pragma once

#include "Vulk.h"

struct VulkDescriptorSet
{
    Vulk &vk;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    VulkDescriptorSet(Vulk &vk, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> &&descriptorSets)
        : vk(vk),
          descriptorSetLayout(descriptorSetLayout),
          descriptorPool(descriptorPool),
          descriptorSets(std::move(descriptorSets)) {}

    ~VulkDescriptorSet()
    {
        vkDestroyDescriptorPool(vk.device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(vk.device, descriptorSetLayout, nullptr);
    }
};

class VulkDescriptorSetBuilder
{
    Vulk &vk;
    VulkDescriptorSetLayoutBuilder layoutBuilder;
    VulkDescriptorPoolBuilder poolBuilder;
    struct BufSetUpdaterInfo
    {
        VkBuffer buf;
        VkDeviceSize range;
    };

    struct PerFrameInfo
    {
        std::unordered_map<VulkShaderUBOBindings, BufSetUpdaterInfo> uniformSetInfos;
        std::unordered_map<VulkShaderSSBOBindings, BufSetUpdaterInfo> ssboSetInfos;
    };
    std::array<PerFrameInfo, MAX_FRAMES_IN_FLIGHT> perFrameInfos;

    struct SamplerSetUpdaterInfo
    {
        VkImageView imageView;
        VkSampler sampler;
    };
    std::unordered_map<VulkShaderTextureBindings, SamplerSetUpdaterInfo> samplerSetInfos;

public:
    VulkDescriptorSetBuilder(Vulk &vk) : vk(vk), layoutBuilder(vk), poolBuilder(vk) {}

    template <typename T>
    VulkDescriptorSetBuilder &addUniformBuffers(std::array<VulkUniformBuffer<T>, MAX_FRAMES_IN_FLIGHT> &uniformBuffers, VkShaderStageFlags stageFlags, VulkShaderUBOBindings bindingID)
    {
        layoutBuilder.addUniformBuffer(bindingID, stageFlags);
        poolBuilder.addUniformBufferCount(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            perFrameInfos[i].uniformSetInfos[bindingID] = {uniformBuffers[i].buf, sizeof(T)};
        }
        return *this;
    }

    VulkDescriptorSetBuilder &addImageSampler(VkShaderStageFlags stageFlags, VulkShaderTextureBindings bindingID, VkImageView imageView, VkSampler sampler)
    {
        layoutBuilder.addSampler(bindingID, stageFlags);
        poolBuilder.addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT);
        samplerSetInfos[bindingID] = {imageView, sampler};
        return *this;
    }

    std::unique_ptr<VulkDescriptorSet> build()
    {
        VkDescriptorSetLayout descriptorSetLayout = layoutBuilder.build();
        VkDescriptorPool pool = poolBuilder.build(MAX_FRAMES_IN_FLIGHT);
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            descriptorSets[i] = vk.createDescriptorSet(descriptorSetLayout, pool);
            VulkDescriptorSetUpdater updater = VulkDescriptorSetUpdater(descriptorSets[i]);

            for (auto &pair : perFrameInfos[i].uniformSetInfos)
            {
                updater.addUniformBuffer(pair.second.buf, pair.second.range, pair.first);
            }
            for (auto &pair : perFrameInfos[i].ssboSetInfos)
            {
                updater.addStorageBuffer(pair.second.buf, pair.second.range, pair.first);
            }
            for (auto &pair : samplerSetInfos)
            {
                updater.addImageSampler(pair.second.imageView, pair.second.sampler, pair.first);
            }

            updater.update(vk.device);
        }
        return std::make_unique<VulkDescriptorSet>(vk, descriptorSetLayout, pool, std::move(descriptorSets));
    }
};