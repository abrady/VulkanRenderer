#pragma once

#include "Vulk/Vulk.h"
#include "Vulk/VulkGeo.h"
#include "Vulk/VulkActor.h"
#include "Vulk/VulkCamera.h"
#include "Vulk/VulkPipelineBuilder.h"
#include "Vulk/VulkDescriptorPoolBuilder.h"
#include "Vulk/VulkUniformBuffer.h"
#include "Vulk/VulkStorageBuffer.h"
#include "Vulk/VulkDescriptorSetUpdater.h"
#include "Vulk/VulkTextureView.h"
#include "Vulk/VulkDescriptorSetBuilder.h"
#include "Vulk/VulkBufferBuilder.h"
#include "Vulk/VulkResources.h"

class PlanarShadowWorld
{
public:
    Vulk &vk;
    VulkCamera camera;
    VulkResources resources;

public:
    PlanarShadowWorld(Vulk &vk) : vk(vk),
                                  resources(vk)
    {
        // load resources
        resources.loadActor("Skull");

        // set up render globals
        for (auto &light : resources.modelUBOs.lights)
        {
            light.mappedUBO->pos = glm::vec3(0.0f, 0.0f, 0.0f);
            light.mappedUBO->color = glm::vec3(1.0f, 1.0f, 1.0f);
        }
        camera.lookAt(glm::vec3(.9f, 1.f, 1.3f), glm::vec3(0.5f, 0.f, 0.f));
    }

    void updateXformsUBO(VulkResources::XformsUBO &ubo, VkViewport const &viewport)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        time = 0.0f;
        ubo.world = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 fwd = camera.getForwardVec();
        glm::vec3 lookAt = camera.eye + fwd;
        glm::vec3 up = camera.getUpVec();
        ubo.view = glm::lookAt(camera.eye, lookAt, up);
        ubo.proj = glm::perspective(glm::radians(45.0f), viewport.width / (float)viewport.height, .01f, 10.0f);
        ubo.proj[1][1] *= -1;
    }

    void render(VkCommandBuffer commandBuffer, uint32_t currentFrame, VkViewport const &viewport, VkRect2D const &scissor)
    {
        updateXformsUBO(*resources.modelUBOs.xforms[currentFrame], viewport);
        *resources.modelUBOs.eyePos[currentFrame] = camera.eye;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, resources.getPipeline("LitModel"));
        VulkMeshResources const *sk = resources.getMesh("Skull");
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, resources.getPipelineLayout("LitModel"), 0, 1, &sk->dsInfo->descriptorSets[currentFrame], 0, nullptr);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sk->vertBuf.buf, offsets);
        vkCmdBindIndexBuffer(commandBuffer, sk->indexBuf.buf, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, sk->numIndices, 1, 0, 0, 0);
    }

    ~PlanarShadowWorld() {}
};
