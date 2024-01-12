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

class MirrorWorld
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

    struct FrameUBOs
    {
        Vulk &vk;
        std::array<VulkUniformBuffer<XformsUBO>, MAX_FRAMES_IN_FLIGHT> xforms;
        std::array<VulkUniformBuffer<glm::vec3>, MAX_FRAMES_IN_FLIGHT> eyePos;
        VulkUniformBuffer<VulkLight> light;
        FrameUBOs(Vulk &vk) : vk(vk)
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
        ~FrameUBOs()
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
    FrameUBOs frameUBOs;

    VulkTextureView skullTextureView, skullNormalView;
    VkSampler textureSampler;

    std::unique_ptr<VulkMeshRender> skull;
    VkImage mirrorImage;
    VkDeviceMemory mirrorImageMemory;
    VkImageView mirrorImageView;
public:
    MirrorWorld(Vulk &vk) : vk(vk),
                            frameUBOs(vk),
                            skullTextureView(vk, "Assets/Models/Skull/DiffuseMap.png"),
                            skullNormalView(vk, "Assets/Models/Skull/NormalMap.png")
    {
        frameUBOs.light.mappedUBO->pos = glm::vec3(2.0f, .5f, .5f);
        frameUBOs.light.mappedUBO->color = glm::vec3(.7f, .7f, .7f);

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

        VulkPipelineBuilder(vk)
            .addVertexShaderStage("Source/Shaders/Vert/LitModel.spv")
            .addFragmentShaderStage("Source/Shaders/Frag/LitModel.spv")
            .addVulkVertexInput(0)
            .setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setCullMode(VK_CULL_MODE_BACK_BIT)
            .setDepthTestEnabled(true)
            .setDepthWriteEnabled(true)
            .setBlendingEnabled(true)
            .build(skull->descriptorSetLayout, &skull->pipelineLayout, &skull->pipeline);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            skull->descriptorSets[i] = vk.createDescriptorSet(skull->descriptorSetLayout, skull->descriptorPool);
            VulkDescriptorSetUpdater(skull->descriptorSets[i])
                .addUniformBuffer(frameUBOs.xforms[i].buf, frameUBOs.xforms[i].getSize(), VulkShaderBinding_XformsUBO)
                .addUniformBuffer(frameUBOs.eyePos[i].buf, frameUBOs.eyePos[i].getSize(), VulkShaderBinding_EyePos)
                .addUniformBuffer(frameUBOs.light.buf, frameUBOs.light.getSize(), VulkShaderBinding_Lights)
                .addImageSampler(skullTextureView.textureImageView, textureSampler, VulkShaderBinding_TextureSampler)
                .addImageSampler(skullNormalView.textureImageView, textureSampler, VulkShaderBinding_NormalSampler)
                .update(vk.device);
        }

        // Create a VkPipelineDepthStencilStateCreateInfo structure
        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_TRUE;

        
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
        updateXformsUBO(*frameUBOs.xforms[currentFrame].mappedUBO, viewport);
        *frameUBOs.eyePos[currentFrame].mappedUBO = camera.eye;

        // skull
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skull->pipeline);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &skull->vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, skull->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skull->pipelineLayout, 0, 1, &skull->descriptorSets[currentFrame], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, (uint32_t)skull->numIndices, 1, 0, 0, 0);
    }

    ~MirrorWorld()
    {
        vkDestroySampler(vk.device, textureSampler, nullptr);
    }
};
