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
#include "Vulk/VulkMeshRender.h"

class OutlineWorld
{
public:
    Vulk &vk;
    VulkCamera camera;

    struct XformsUBO
    {
        alignas(16) glm::mat4 world;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct SkullUBOs
    {
        Vulk &vk;
        std::array<VulkUniformBuffer<XformsUBO>, MAX_FRAMES_IN_FLIGHT> xforms;
        std::array<VulkUniformBuffer<glm::vec3>, MAX_FRAMES_IN_FLIGHT> eyePos;
        VulkUniformBuffer<VulkLight> light;
        SkullUBOs(Vulk &vk) : vk(vk)
        {
            for (auto &ubo : xforms)
            {
                ubo.init(vk);
            }
            for (auto &ubo : eyePos)
            {
                ubo.init(vk);
            }
            light.init(vk);
        }
        ~SkullUBOs()
        {
            for (auto &ubo : xforms)
            {
                ubo.cleanup(vk);
            }
            for (auto &ubo : eyePos)
            {
                ubo.cleanup(vk);
            }
            light.cleanup(vk);
        }
    };
    SkullUBOs skullUBOs;

    VulkTextureView skullTextureView,
        skullNormalView;
    VkSampler textureSampler;

    std::unique_ptr<VulkMeshRender> skull;

    struct Outline
    {
        Vulk &vk;
        std::array<VulkUniformBuffer<glm::mat4>, MAX_FRAMES_IN_FLIGHT> xforms;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        VkDescriptorPool descriptorPool;
        VkDescriptorSetLayout descriptorSetLayout;
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
        Outline(Vulk &vk) : vk(vk)
        {
            for (auto &ubo : xforms)
            {
                ubo.init(vk);
            }
        }
        ~Outline()
        {
            for (auto &ubo : xforms)
            {
                ubo.cleanup(vk);
            }
            vkDestroyPipeline(vk.device, pipeline, nullptr);
            vkDestroyPipelineLayout(vk.device, pipelineLayout, nullptr);
            vkDestroyDescriptorPool(vk.device, descriptorPool, nullptr);
            vkDestroyDescriptorSetLayout(vk.device, descriptorSetLayout, nullptr);
        }
    } outline;

public:
    OutlineWorld(Vulk &vk) : vk(vk),
                             skullUBOs(vk),
                             skullTextureView(vk, "Assets/Models/Skull/DiffuseMap.png"),
                             skullNormalView(vk, "Assets/Models/Skull/NormalMap.png"),
                             outline(vk)
    {
        skullUBOs.light.mappedUBO->pos = glm::vec3(2.0f, .5f, .5f);
        skullUBOs.light.mappedUBO->color = glm::vec3(.7f, .7f, .7f);

        textureSampler = vk.createTextureSampler();
        camera.lookAt(glm::vec3(0.0f, 0.f, 1.3f), glm::vec3(0.f, 0.f, 0.f));

        skull = std::make_unique<VulkMeshRender>(vk, VulkMesh::loadFromFile("Assets/Models/Skull/Skull.obj"));
        skull->descriptorSetLayout = VulkDescriptorSetLayoutBuilder(vk)
                                         .addUniformBuffer(VulkShaderBinding_XformsUBO, VK_SHADER_STAGE_VERTEX_BIT)
                                         .addUniformBuffer(VulkShaderBinding_Lights, VK_SHADER_STAGE_FRAGMENT_BIT)
                                         .addUniformBuffer(VulkShaderBinding_EyePos, VK_SHADER_STAGE_FRAGMENT_BIT)
                                         .addSampler(VulkShaderBinding_TextureSampler)
                                         .addSampler(VulkShaderBinding_NormalSampler)
                                         .build();

        skull->descriptorPool = VulkDescriptorPoolBuilder(vk)
                                    .addUniformBufferCount(MAX_FRAMES_IN_FLIGHT * 3)
                                    .addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT * 2)
                                    .build(MAX_FRAMES_IN_FLIGHT);

        // we do two passes for an outline:
        // pass 1: draw the model with stencil buffer enabled, and write to the stencil buffer
        // pass 2: draw the model again scaled up slightly, but only where the stencil buffer is not set and with the depth test write disabled
        VulkPipelineBuilder(vk)
            .addVertexShaderStage("Source/Shaders/Vert/LitModel.spv")
            .addFragmentShaderStage("Source/Shaders/Frag/LitModel.spv")
            .addVulkVertexInput(0)
            .setStencilTestEnabled(true)
            .setStencilFrontFailOp(VK_STENCIL_OP_KEEP)      // keep the stencil buffer unchanged
            .setFrontStencilPassOp(VK_STENCIL_OP_REPLACE)   // replace the stencil buffer with the reference value
            .setFrontStencilCompareOp(VK_COMPARE_OP_ALWAYS) // always pass the stencil test for our first pass
            .setFrontStencilCompareMask(0xFF)               // compare against all bits
            .setFrontStencilWriteMask(0xFF)                 // allow write to all bits
            .setFrontStencilReference(1)                    // value to write to the stencil buffer
            .copyFrontStencilToBack()                       // we cull back facing triangles, but putting this here to show it exists even though it's not used
            .build(skull->descriptorSetLayout, &skull->pipelineLayout, &skull->graphicsPipeline);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            skull->descriptorSets[i] = vk.createDescriptorSet(skull->descriptorSetLayout, skull->descriptorPool);
            VulkDescriptorSetUpdater(skull->descriptorSets[i])
                .addUniformBuffer(skullUBOs.xforms[i].buf, skullUBOs.xforms[i].getSize(), VulkShaderBinding_XformsUBO)
                .addUniformBuffer(skullUBOs.eyePos[i].buf, skullUBOs.eyePos[i].getSize(), VulkShaderBinding_EyePos)
                .addUniformBuffer(skullUBOs.light.buf, skullUBOs.light.getSize(), VulkShaderBinding_Lights)
                .addImageSampler(skullTextureView.textureImageView, textureSampler, VulkShaderBinding_TextureSampler)
                .addImageSampler(skullNormalView.textureImageView, textureSampler, VulkShaderBinding_NormalSampler)
                .update(vk.device);
        }

        // pass 2
        outline.descriptorSetLayout = VulkDescriptorSetLayoutBuilder(vk)
                                          .addUniformBuffer(VulkShaderBinding_XformsUBO, VK_SHADER_STAGE_VERTEX_BIT)
                                          .addUniformBuffer(VulkShaderBinding_ModelXform, VK_SHADER_STAGE_VERTEX_BIT)
                                          .build();

        VulkPipelineBuilder(vk)
            .addVertexShaderStage("Source/Shaders/Vert/Outline.spv")
            .addFragmentShaderStage("Source/Shaders/Frag/Outline.spv")
            .addVulkVertexInput(0)
            .setDepthTestEnabled(true)
            .setDepthWriteEnabled(false)
            .setStencilTestEnabled(true)
            .setStencilFrontFailOp(VK_STENCIL_OP_KEEP)         // keep the stencil buffer unchanged
            .setFrontStencilPassOp(VK_STENCIL_OP_KEEP)         // keep the stencil buffer unchanged
            .setFrontStencilCompareOp(VK_COMPARE_OP_NOT_EQUAL) // only modify where the model did not draw
            .setFrontStencilCompareMask(0xFF)                  // compare against all bits
            .setFrontStencilWriteMask(0x00)                    // don't write to the stencil buffer
            .setFrontStencilReference(1)                       // this is the value we're comparing against that was set in the first pass
            .copyFrontStencilToBack()                          // we cull back facing triangles, etc.
            .build(outline.descriptorSetLayout, &outline.pipelineLayout, &outline.pipeline);

        outline.descriptorPool = VulkDescriptorPoolBuilder(vk)
                                     .addUniformBufferCount(MAX_FRAMES_IN_FLIGHT * 2)
                                     .build(MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            outline.descriptorSets[i] = vk.createDescriptorSet(outline.descriptorSetLayout, outline.descriptorPool);
            VulkDescriptorSetUpdater(outline.descriptorSets[i])
                .addUniformBuffer(skullUBOs.xforms[i].buf, skullUBOs.xforms[i].getSize(), VulkShaderBinding_XformsUBO)
                .addUniformBuffer(outline.xforms[i].buf, outline.xforms[i].getSize(), VulkShaderBinding_ModelXform)
                .update(vk.device);

            *outline.xforms[i].mappedUBO = glm::scale(glm::mat4(1.0f), glm::vec3(1.1f));
        }
    }

    void updateXformsUBO(XformsUBO &ubo, VkViewport const &viewport)
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
        VkDeviceSize offsets[] = {0};
        updateXformsUBO(*skullUBOs.xforms[currentFrame].mappedUBO, viewport);
        *skullUBOs.eyePos[currentFrame].mappedUBO = camera.eye;

        // skull
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skull->graphicsPipeline);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &skull->vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, skull->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skull->pipelineLayout, 0, 1, &skull->descriptorSets[currentFrame], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, (uint32_t)skull->numIndices, 1, 0, 0, 0);

        // outline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, outline.pipeline);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &skull->vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, skull->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, outline.pipelineLayout, 0, 1, &outline.descriptorSets[currentFrame], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, (uint32_t)skull->numIndices, 1, 0, 0, 0);
    }

    ~OutlineWorld()
    {
        vkDestroySampler(vk.device, textureSampler, nullptr);
    }
};
