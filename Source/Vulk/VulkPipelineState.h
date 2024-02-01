#pragma once

#include "Vulk.h"
#include "VulkPipelineState.h"
#include "VulkDescriptorSetBuilder.h"
#include "VulkPipelineBuilder.h"

class VulkPipelineState
{
    std::unique_ptr<VulkDescriptorSet> dsInfo;
    std::unique_ptr<VulkPipeline> plInfo;
    Vulk &vk;

public:
    VulkPipelineState(Vulk &vk, std::unique_ptr<VulkDescriptorSet> &&dsInfo, std::unique_ptr<VulkPipeline> &&plInfo)
        : vk(vk),
          dsInfo(std::move(dsInfo)),
          plInfo(std::move(plInfo)) {}

    void setRenderState(VkCommandBuffer commandBuffer, uint32_t currentFrame, VkViewport const &viewport, VkRect2D const &scissor)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, plInfo->pipeline);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, plInfo->pipelineLayout, 0, 1, &dsInfo->descriptorSets[currentFrame], 0, nullptr);
    }
};
